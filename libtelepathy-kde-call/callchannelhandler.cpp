/*
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "callchannelhandler_p.h"
#include "deviceelementfactory_p.h"
#include <QGlib/Signal>
#include <QGst/ElementFactory>
#include <QGst/Bus>
#include <QGst/GhostPad>
#include <QGst/Message>
#include <KDebug>
#include <KLocalizedString>
#include <KStandardDirs>
#include <TelepathyQt4/Connection>

QGLIB_REGISTER_TYPE(GList*)
QGLIB_REGISTER_TYPE_IMPLEMENTATION(GList*, fs_codec_list_get_type())
QGLIB_REGISTER_VALUEIMPL_FOR_BOXED_TYPE(GList*)


CallChannelHandler::CallChannelHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent)
    : QObject(parent), d(new CallChannelHandlerPrivate(this))
{
    d->init(channel);
    qRegisterMetaType<CallParticipant*>();
}

CallChannelHandler::~CallChannelHandler()
{
    delete d;
}

QList<CallParticipant*> CallChannelHandler::participants() const
{
    return d->participants();
}

void CallChannelHandler::hangup(const QString & message)
{
    //no reason to handle the return value. invalidated() will be emited...
    d->channel()->hangupCall(Tp::StreamedMediaChannel::StateChangeReasonUserRequested, QString(), message);
}

void CallChannelHandlerPrivate::init(const Tp::StreamedMediaChannelPtr & channel)
{
    m_channel = channel;
    m_participantData[Myself] = new ParticipantData(this, Myself);
    m_participantData[RemoteContact] = new ParticipantData(this, RemoteContact);

    connect(m_channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*, QString, QString)));

    //initialize gstreamer
    static bool gstInitDone = false;
    if (!gstInitDone) {
        QGst::init();
        gstInitDone = true;
    }

    m_pipeline = QGst::Pipeline::create();
    m_pipeline->setState(QGst::StatePlaying);

    TfChannel *tfChannel = Tp::createFarsightChannel(channel);
    Q_ASSERT(tfChannel);
    m_tfChannel = QGlib::ObjectPtr::wrap(reinterpret_cast<GObject*>(tfChannel), false);

    /* Set up the telepathy farsight channel */
    QGlib::Signal::connect(m_tfChannel, "closed",
                           this, &CallChannelHandlerPrivate::onTfChannelClosed);
    QGlib::Signal::connect(m_tfChannel, "session-created",
                           this, &CallChannelHandlerPrivate::onSessionCreated);
    QGlib::Signal::connect(m_tfChannel, "stream-created",
                           this, &CallChannelHandlerPrivate::onStreamCreated);
    QGlib::Signal::connect(m_tfChannel, "stream-get-codec-config",
                           this, &CallChannelHandlerPrivate::onStreamGetCodecConfig);
    QGlib::Signal::connect(m_pipeline->bus(), "message",
                           this, &CallChannelHandlerPrivate::onBusMessage);
}

Tp::ContactPtr CallChannelHandlerPrivate::contactOfParticipant(Who who) const
{
    if (who == Myself) {
        return m_channel->groupSelfContact();
    } else {
        Tp::Contacts contacts = m_channel->groupContacts();
        Tp::ContactPtr contact;
        Q_FOREACH(const Tp::ContactPtr & c, contacts) {
            if (c != m_channel->groupSelfContact()) {
                contact = c;
            }
        }
        if (!contact) {
            kWarning() << "No remote contact available, returning NULL";
        }
        return contact;
    }
}

void CallChannelHandlerPrivate::onChannelInvalidated(Tp::DBusProxy *proxy,
                                                     const QString & errorName,
                                                     const QString & errorMessage)
{
    Q_UNUSED(proxy);
    kDebug() << "Channel invalidated:" << errorName << errorMessage;

    Q_EMIT q->callEnded(errorMessage);
}


void CallChannelHandlerPrivate::onSessionCreated(QGst::ElementPtr & conference)
{
    kDebug();
    m_pipeline->bus()->addSignalWatch();
    m_conference = conference;
    m_pipeline->add(conference);
    conference->setState(QGst::StatePlaying);
}

void CallChannelHandlerPrivate::onTfChannelClosed()
{
    kDebug();
    m_pipeline->bus()->removeSignalWatch();
    m_pipeline->setState(QGst::StateNull);
}

void CallChannelHandlerPrivate::onBusMessage(const QGst::MessagePtr & message)
{
    TfChannel *c = reinterpret_cast<TfChannel*>(static_cast<GObject*>(m_tfChannel));
    tf_channel_bus_message(c, message);
}

void CallChannelHandlerPrivate::onStreamCreated(const QGlib::ObjectPtr & stream)
{
    QGlib::Signal::connect(stream, "request-resource", this,
                           &CallChannelHandlerPrivate::onRequestResource, QGlib::Signal::PassSender);
    QGlib::Signal::connect(stream, "src-pad-added", this,
                           &CallChannelHandlerPrivate::onSrcPadAdded, QGlib::Signal::PassSender);
    QGlib::Signal::connect(stream, "free-resource", this,
                           &CallChannelHandlerPrivate::onFreeResource, QGlib::Signal::PassSender);
}

GList *CallChannelHandlerPrivate::onStreamGetCodecConfig()
{
    QString codecsFile = KStandardDirs::locate("data", "libtelepathy-kde-call/codec-preferences.ini");
    if (!codecsFile.isEmpty()) {
        kDebug() << "Reading codec preferences from" << codecsFile;
        return fs_codec_list_from_keyfile(QFile::encodeName(codecsFile), NULL);
    } else {
        return NULL;
    }
}


bool CallChannelHandlerPrivate::onRequestResource(const QGlib::ObjectPtr & stream, uint direction)
{
    bool audio = stream->property("media-type").get<uint>() == Tp::MediaStreamTypeAudio;

    switch(direction) {
    case Tp::MediaStreamDirectionSend:
    {
        kDebug() << "Opening" << (audio ? "audio" : "video") << "input device";

        QGst::ElementPtr src = audio ? DeviceElementFactory::makeAudioCaptureElement()
                                     : DeviceElementFactory::makeVideoCaptureElement();
        if (!src) {
            Q_EMIT q->error(audio ? i18n("The audio input device could not be initialized")
                                  : i18n("The video input device could not be initialized"));
            return false;
        }

        if (!(audio ? createAudioBin(m_participantData[Myself])
                    : createVideoBin(m_participantData[Myself], true)))
        {
            Q_EMIT q->error(i18n("Could not create some of the required elements. "
                                 "Please check your GStreamer installation."));
            return false;
        }

        (audio ? m_audioInputDevice : m_videoInputDevice) = src;

        QGst::BinPtr & bin = audio ? m_participantData[Myself]->audioBin
                                   : m_participantData[Myself]->videoBin;

        m_pipeline->add(src);
        m_pipeline->add(bin);

        src->link(bin);
        bin->getStaticPad("src")->link(stream->property("sink-pad").get<QGst::PadPtr>());

        //set everything to playing state
        bin->setState(QGst::StatePlaying);
        src->setState(QGst::StatePlaying);

        //create the participant if not already there
        if (!m_participants.contains(Myself)) {
            m_participants[Myself] = new CallParticipant(m_participantData[Myself], q);
            Q_EMIT q->participantJoined(m_participants[Myself]);
        }

        if (audio) {
            Q_EMIT m_participants[Myself]->audioStreamAdded(m_participants[Myself]);
        } else {
            Q_EMIT m_participants[Myself]->videoStreamAdded(m_participants[Myself]);
        }

        break;
    }
    case Tp::MediaStreamDirectionReceive:
    {
        kDebug() << "Opening" << (audio ? "audio" : "video") << "output device";

        if (audio) {
            QGst::ElementPtr element = DeviceElementFactory::makeAudioOutputElement();
            if ( !element ) {
                Q_EMIT q->error(i18n("The audio output device could not be initialized"));
                return false;
            }
            m_audioOutputDevice = element;
        }

        if (!(audio ? createAudioBin(m_participantData[RemoteContact])
                    : createVideoBin(m_participantData[RemoteContact], false)))
        {
            Q_EMIT q->error(i18n("Could not create some of the required elements. "
                                 "Please check your GStreamer installation."));
            return false;
        }

        if (audio) {
            m_pipeline->add(m_audioOutputDevice);
            m_pipeline->add(m_participantData[RemoteContact]->audioBin);
            m_participantData[RemoteContact]->audioBin->link(m_audioOutputDevice);
            m_audioOutputDevice->setState(QGst::StatePlaying);
            m_participantData[RemoteContact]->audioBin->setState(QGst::StatePlaying);
        } else {
            m_pipeline->add(m_participantData[RemoteContact]->videoBin);
            m_participantData[RemoteContact]->videoBin->setState(QGst::StatePlaying);
        }

        break;
    }
    default:
        Q_ASSERT(false);
    }

    return true;
}

/* WARNING this slot is called in a different thread. */
void CallChannelHandlerPrivate::onSrcPadAdded(const QGlib::ObjectPtr & stream, QGst::PadPtr & pad)
{
    bool audio = stream->property("media-type").get<uint>() == Tp::MediaStreamTypeAudio;
    QGst::BinPtr & bin = audio ? m_participantData[RemoteContact]->audioBin
                               : m_participantData[RemoteContact]->videoBin;

    kDebug() << (audio ? "Audio" : "Video") << "src pad added";

    pad->link(bin->getStaticPad("sink"));

    //create the participant if not already there
    if (!m_participants.contains(RemoteContact)) {
        m_participants[RemoteContact] = new CallParticipant(m_participantData[RemoteContact]);
        m_participants[RemoteContact]->moveToThread(q->thread());
        m_participants[RemoteContact]->setParent(q);
        QMetaObject::invokeMethod(q, "participantJoined", Qt::QueuedConnection,
                                  Q_ARG(CallParticipant*, m_participants[RemoteContact]));
    }

    const char *streamAddedSignal = audio ? "audioStreamAdded" : "videoStreamAdded";
    QMetaObject::invokeMethod(m_participants[RemoteContact], streamAddedSignal, Qt::QueuedConnection,
                              Q_ARG(CallParticipant*, m_participants[RemoteContact]));
}

void CallChannelHandlerPrivate::onFreeResource(const QGlib::ObjectPtr & stream, uint direction)
{
    bool audio = stream->property("media-type").get<uint>() == Tp::MediaStreamTypeAudio;
    Who who = direction == Tp::MediaStreamDirectionSend ? Myself : RemoteContact;

    QGst::BinPtr & bin = audio ? m_participantData[who]->audioBin
                               : m_participantData[who]->videoBin;
    QGst::ElementPtr & element = audio ? (who == Myself ? m_audioInputDevice : m_audioOutputDevice)
                                       : m_videoInputDevice;

    kDebug() << "Closing" << (audio ? "audio" : "video")
             << ((who == Myself) ? "input" : "output") << "device";

    bin->setState(QGst::StateNull);
    if (audio || who != RemoteContact) {
        element->setState(QGst::StateNull);
    }

    m_pipeline->remove(bin);
    if (audio || who != RemoteContact) {
        m_pipeline->remove(element);
    }

    //drop references to destroy the elements
    if (audio) {
        m_participantData[who]->volumeControl.clear();
    } else {
        m_participantData[who]->colorBalanceControl.clear();
        m_participantData[who]->videoWidget.data()->setVideoSink(QGst::ElementPtr());
    }
    bin.clear();
    if (audio || who != RemoteContact) {
        element.clear();
    }

    if (audio) {
        Q_EMIT m_participants[who]->audioStreamRemoved(m_participants[who]);
    } else {
        Q_EMIT m_participants[who]->videoStreamRemoved(m_participants[who]);
    }

    //remove the participant if it has no other stream on it
    if ((audio && !m_participants[who]->hasVideoStream()) ||
        (!audio && !m_participants[who]->hasAudioStream()))
    {
        CallParticipant *p = m_participants.take(who);
        Q_EMIT q->participantLeft(p);
        p->deleteLater();
    }
}


bool CallChannelHandlerPrivate::createAudioBin(QExplicitlySharedDataPointer<ParticipantData> & data)
{
    QGst::BinPtr bin = QGst::Bin::create();
    QGst::ElementPtr volume = QGst::ElementFactory::make("volume");
    QGst::ElementPtr audioConvert = QGst::ElementFactory::make("audioconvert");
    QGst::ElementPtr audioResample = QGst::ElementFactory::make("audioresample");

    if (!bin || !volume || !audioConvert || !audioResample) {
        kDebug() << "Could not make some of the audio bin elements";
        return false;
    }

    if (!(data->volumeControl = volume.dynamicCast<QGst::StreamVolume>())) {
        kDebug() << "Could not cast volume element to GstStreamVolume";
        return false;
    }
    data->audioBin = bin;

    bin->add(volume);
    bin->add(audioConvert);
    bin->add(audioResample);

    bin->addPad(QGst::GhostPad::create(volume->getStaticPad("sink"), "sink"));
    volume->link(audioConvert);
    audioConvert->link(audioResample);
    bin->addPad(QGst::GhostPad::create(audioResample->getStaticPad("src"), "src"));
    return true;
}

bool CallChannelHandlerPrivate::createVideoBin(QExplicitlySharedDataPointer<ParticipantData> & data, bool withSink)
{
    QGst::BinPtr bin = QGst::Bin::create();
    QGst::ElementPtr videoBalance = QGst::ElementFactory::make("videobalance");
    QGst::ElementPtr tee = QGst::ElementFactory::make("tee");
    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");
    QGst::ElementPtr colorspace = QGst::ElementFactory::make("ffmpegcolorspace");
    QGst::ElementPtr videoscale = QGst::ElementFactory::make("videoscale");
    QGst::ElementPtr videosink = QGst::ElementFactory::make("qwidgetvideosink"); //FIXME not always the best sink

    if (!bin || !videoBalance || !tee || !queue || !colorspace || !videoscale || !videosink) {
        kDebug() << "Could not make some of the video bin elements";
        return false;
    }

    if (!(data->colorBalanceControl = videoBalance.dynamicCast<QGst::ColorBalance>())) {
        kDebug() << "Could not cast videobalance element to GstColorBalance";
        return false;
    }
    data->videoBin = bin;

    videosink->setProperty("force-aspect-ratio", true); //FIXME should be externally configurable

    bin->add(videoBalance);
    bin->add(tee);
    bin->add(queue);
    bin->add(colorspace);
    bin->add(videoscale);
    bin->add(videosink);

    bin->addPad(QGst::GhostPad::create(videoBalance->getStaticPad("sink"), "sink"));
    videoBalance->link(tee);
    tee->getRequestPad("src%d")->link(queue->getStaticPad("sink"));
    queue->link(colorspace);
    colorspace->link(videoscale);
    videoscale->link(videosink);

    data->videoWidget = new QGst::Ui::VideoWidget;
    data->videoWidget.data()->setVideoSink(videosink);

    if (withSink) {
        QGst::ElementPtr queue2 = QGst::ElementFactory::make("queue");
        bin->add(queue2);
        tee->getRequestPad("src%d")->link(queue2->getStaticPad("sink"));

        bin->addPad(QGst::GhostPad::create(queue2->getStaticPad("src"), "src"));
    }

    return true;
}

#include "callchannelhandler.moc"
#include "callchannelhandler_p.moc"
