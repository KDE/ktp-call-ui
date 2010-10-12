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
#include "../libqtpfarsight/qtfchannel.h"
#include <QGst/ElementFactory>
#include <QGst/Bus>
#include <QGst/GhostPad>
#include <KDebug>
#include <KLocalizedString>
#include <KStandardDirs>
#include <TelepathyQt4/Connection>


#if 0
static Tp::ContactPtr findRemoteContact(const Tp::ChannelPtr & channel)
{
    //HACK assuming we only have one other member, which is always the case with the StreamedMedia interface
    Tp::Contacts contacts = channel->groupContacts();
    Tp::ContactPtr contact;
    Q_FOREACH(const Tp::ContactPtr & c, contacts) {
        if (c != channel->groupSelfContact()) {
            contact = c;
        }
    }
    Q_ASSERT(contact);
    return contact;
}
#endif


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

    //initialize gstreamer
    static bool gstInitDone = false;
    if ( !gstInitDone ) {
        QGst::init();
        gstInitDone = true;
    }

    m_pipeline = QGst::Pipeline::create();
    m_pipeline->setState(QGst::StatePlaying);

    connect(m_channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*, QString, QString)));

    m_qtfchannel = new QTfChannel(channel, m_pipeline->bus(), this);
    connect(m_qtfchannel, SIGNAL(sessionCreated(QGst::ElementPtr)), SLOT(onSessionCreated(QGst::ElementPtr)));
    connect(m_qtfchannel, SIGNAL(closed()), SLOT(onQTfChannelClosed()));

    connect(m_qtfchannel, SIGNAL(openAudioInputDevice(bool*)), SLOT(openAudioInputDevice(bool*)));
    connect(m_qtfchannel, SIGNAL(audioSinkPadAdded(QGst::PadPtr)), SLOT(audioSinkPadAdded(QGst::PadPtr)));
    connect(m_qtfchannel, SIGNAL(closeAudioInputDevice()), SLOT(closeAudioInputDevice()));

    connect(m_qtfchannel, SIGNAL(openVideoInputDevice(bool*)), SLOT(openVideoInputDevice(bool*)));
    connect(m_qtfchannel, SIGNAL(videoSinkPadAdded(QGst::PadPtr)), SLOT(videoSinkPadAdded(QGst::PadPtr)));
    connect(m_qtfchannel, SIGNAL(closeVideoInputDevice()), SLOT(closeVideoInputDevice()));

    connect(m_qtfchannel, SIGNAL(openAudioOutputDevice(bool*)), SLOT(openAudioOutputDevice(bool*)));
    connect(m_qtfchannel, SIGNAL(audioSrcPadAdded(QGst::PadPtr)),
            SLOT(audioSrcPadAdded(QGst::PadPtr)), Qt::DirectConnection);
    connect(m_qtfchannel, SIGNAL(closeAudioOutputDevice()), SLOT(closeAudioOutputDevice()));

    connect(m_qtfchannel, SIGNAL(openVideoOutputDevice(bool*)), SLOT(openVideoOutputDevice(bool*)));
    connect(m_qtfchannel, SIGNAL(videoSrcPadAdded(QGst::PadPtr)),
            SLOT(videoSrcPadAdded(QGst::PadPtr)), Qt::DirectConnection);
    connect(m_qtfchannel, SIGNAL(closeVideoOutputDevice()), SLOT(closeVideoOutputDevice()));

    QString codecsFile = KStandardDirs::locate("data", "libtelepathy-kde-call/codec-preferences.ini");
    if ( !codecsFile.isEmpty() ) {
        kDebug() << "Reading codec preferences from" << codecsFile;
        m_qtfchannel->setCodecsConfigFile(codecsFile);
    }

    m_participantData[Myself] = new ParticipantData(this, Myself);
    m_participantData[RemoteContact] = new ParticipantData(this, RemoteContact);
}

void CallChannelHandlerPrivate::onChannelInvalidated(Tp::DBusProxy *proxy,
                                                     const QString & errorName,
                                                     const QString & errorMessage)
{
    Q_UNUSED(proxy);
    kDebug() << "Channel invalidated:" << errorName << errorMessage;

    Q_EMIT q->callEnded(errorMessage);
}

void CallChannelHandlerPrivate::onSessionCreated(QGst::ElementPtr conference)
{
    m_pipeline->add(conference);
    conference->setState(QGst::StatePlaying);
}

void CallChannelHandlerPrivate::onQTfChannelClosed()
{
    m_pipeline->setState(QGst::StateNull);
}

void CallChannelHandlerPrivate::openAudioInputDevice(bool *success)
{
    kDebug() << "Opening audio input device";
    *success = openInputDevice(true);
}

void CallChannelHandlerPrivate::audioSinkPadAdded(QGst::PadPtr sinkPad)
{
    kDebug() << "Audio sink pad added";
    Q_ASSERT(!m_audioInputDevice.isNull());
    sinkPadAdded(sinkPad, true);
}

void CallChannelHandlerPrivate::closeAudioInputDevice()
{
    kDebug() << "Closing audio input device";
    closeInputDevice(true);
}

void CallChannelHandlerPrivate::openVideoInputDevice(bool *success)
{
    kDebug() << "Opening video input device";
    *success = openInputDevice(false);
}

void CallChannelHandlerPrivate::videoSinkPadAdded(QGst::PadPtr sinkPad)
{
    kDebug() << "Video sink pad added";
    Q_ASSERT(!m_videoInputDevice.isNull());
    sinkPadAdded(sinkPad, false);
}

void CallChannelHandlerPrivate::closeVideoInputDevice()
{
    kDebug() << "Closing video input device";
    closeInputDevice(false);
}

void CallChannelHandlerPrivate::openAudioOutputDevice(bool *success)
{
    kDebug() << "Opening audio output device";

    QGst::ElementPtr element = DeviceElementFactory::makeAudioOutputElement();
    if ( !element ) {
        Q_EMIT q->error(i18n("The audio output device could not be initialized"));
        *success = false;
        return;
    }

    createAudioBin(m_participantData[RemoteContact]);

    m_audioOutputDevice = element;
    m_pipeline->add(m_audioOutputDevice);
    m_pipeline->add(m_participantData[RemoteContact]->audioBin);
    m_participantData[RemoteContact]->audioBin->link(m_audioOutputDevice);
    m_audioOutputDevice->setState(QGst::StatePlaying);
    m_participantData[RemoteContact]->audioBin->setState(QGst::StatePlaying);
    *success = true;
}

void CallChannelHandlerPrivate::audioSrcPadAdded(QGst::PadPtr srcPad)
{
    kDebug() << "Audio src pad added";
    Q_ASSERT(!m_audioOutputDevice.isNull());
    srcPadAdded(srcPad, true);
}

void CallChannelHandlerPrivate::closeAudioOutputDevice()
{
    kDebug() << "Audio src pad removed";
    closeOutputDevice(true);
}

void CallChannelHandlerPrivate::openVideoOutputDevice(bool *success)
{
    kDebug() << "Opening video output device";
    //succeed if we have qwidgetvideosink installed
    if (!QGst::ElementFactory::find("qwidgetvideosink")) {
        *success = false;
        return;
    }

    createVideoBin(m_participantData[RemoteContact], false);

    m_pipeline->add(m_participantData[RemoteContact]->videoBin);
    m_participantData[RemoteContact]->videoBin->setState(QGst::StatePlaying);
    *success = true;
}

void CallChannelHandlerPrivate::videoSrcPadAdded(QGst::PadPtr srcPad)
{
    kDebug() << "Video src pad added";
    srcPadAdded(srcPad, false);
}

void CallChannelHandlerPrivate::closeVideoOutputDevice()
{
    kDebug() << "Video src pad removed";
    closeOutputDevice(false);
}


bool CallChannelHandlerPrivate::openInputDevice(bool audio)
{
    QGst::ElementPtr src = audio ? DeviceElementFactory::makeAudioCaptureElement()
                                 : DeviceElementFactory::makeVideoCaptureElement();
    if (!src) {
        Q_EMIT q->error(audio ? i18n("The audio input device could not be initialized")
                              : i18n("The video input device could not be initialized"));
        return false;
    }

    QGst::BinPtr bin;
    if (audio) {
        createAudioBin(m_participantData[Myself]);
        bin = m_participantData[Myself]->audioBin;
        m_audioInputDevice = src;
    } else {
        createVideoBin(m_participantData[Myself], true);
        bin = m_participantData[Myself]->videoBin;
        m_videoInputDevice = src;
    }

    m_pipeline->add(src);
    m_pipeline->add(bin);
    src->link(bin);
    return true;
}

void CallChannelHandlerPrivate::sinkPadAdded(const QGst::PadPtr & sinkPad, bool audio)
{
    //create the participant if not already there
    bool joined = false;
    if (!m_participants.contains(Myself)) {
        m_participants[Myself] = new CallParticipant(m_participantData[Myself], q);
        joined = true;
    }

    QGst::BinPtr & bin = audio ? m_participantData[Myself]->audioBin
                               : m_participantData[Myself]->videoBin;
    QGst::ElementPtr & src = audio ? m_audioInputDevice : m_videoInputDevice;

    //link the pad
    bin->getStaticPad("src")->link(sinkPad);

    //set everything to playing state
    bin->setState(QGst::StatePlaying);
    src->setState(QGst::StatePlaying);

    //if we just created the participant, emit the joined signal
    if (joined) {
        Q_EMIT q->participantJoined(m_participants[Myself]);
    }

    if (audio) {
        Q_EMIT m_participants[Myself]->audioStreamAdded(m_participants[Myself]);
    } else {
        Q_EMIT m_participants[Myself]->videoStreamAdded(m_participants[Myself]);
    }
}

void CallChannelHandlerPrivate::closeInputDevice(bool audio)
{
    QGst::BinPtr & bin = audio ? m_participantData[Myself]->audioBin
                               : m_participantData[Myself]->videoBin;
    QGst::ElementPtr & src = audio ? m_audioInputDevice : m_videoInputDevice;

    bin->setState(QGst::StateNull);
    src->setState(QGst::StateNull);
    m_pipeline->remove(bin);
    m_pipeline->remove(src);

    //drop references to destroy the elements
    if (audio) {
        m_participantData[Myself]->volumeControl.clear();
    } else {
        m_participantData[Myself]->colorBalanceControl.clear();
        m_participantData[Myself]->videoWidget.data()->setVideoSink(QGst::ElementPtr());
    }
    bin.clear();
    src.clear();

    if (audio) {
        Q_EMIT m_participants[Myself]->audioStreamRemoved(m_participants[Myself]);
    } else {
        Q_EMIT m_participants[Myself]->videoStreamRemoved(m_participants[Myself]);
    }

    //remove the participant if it has no other stream on it
    if ((audio && !m_participants[Myself]->hasVideoStream()) ||
        (!audio && !m_participants[Myself]->hasAudioStream()))
    {
        CallParticipant *p = m_participants.take(Myself);
        Q_EMIT q->participantLeft(p);
        p->deleteLater();
    }
}

void CallChannelHandlerPrivate::srcPadAdded(QGst::PadPtr srcPad, bool audio)
{
    //create the participant if not already there
    bool joined = false;
    if (!m_participants.contains(RemoteContact)) {
        m_participants[RemoteContact] = new CallParticipant(m_participantData[RemoteContact]);
        m_participants[RemoteContact]->moveToThread(q->thread());
        m_participants[RemoteContact]->setParent(q);
        joined = true;
    }

    QGst::BinPtr & bin = audio ? m_participantData[RemoteContact]->audioBin
                               : m_participantData[RemoteContact]->videoBin;

    srcPad->link(bin->getStaticPad("sink"));

    //if we just created the participant, emit the joined signal
    if (joined) {
        QMetaObject::invokeMethod(q, "participantJoined", Qt::QueuedConnection,
                                  Q_ARG(CallParticipant*, m_participants[RemoteContact]));
    }

    const char *streamAddedSignal = audio ? "audioStreamAdded" : "videoStreamAdded";
    QMetaObject::invokeMethod(m_participants[RemoteContact], streamAddedSignal, Qt::QueuedConnection,
                              Q_ARG(CallParticipant*, m_participants[RemoteContact]));
}

void CallChannelHandlerPrivate::closeOutputDevice(bool audio)
{
    QGst::BinPtr & bin = audio ? m_participantData[RemoteContact]->audioBin
                               : m_participantData[RemoteContact]->videoBin;

    bin->setState(QGst::StateNull);
    if (audio)
        m_audioOutputDevice->setState(QGst::StateNull);

    m_pipeline->remove(bin);
    if (audio) {
        m_pipeline->remove(m_audioOutputDevice);
    }

    //drop references to destroy the elements
    if (audio) {
        m_participantData[RemoteContact]->volumeControl.clear();
        m_audioOutputDevice.clear();
    } else {
        m_participantData[RemoteContact]->colorBalanceControl.clear();
        m_participantData[RemoteContact]->videoWidget.data()->setVideoSink(QGst::ElementPtr());
    }
    bin.clear();

    if (audio) {
        Q_EMIT m_participants[Myself]->audioStreamRemoved(m_participants[Myself]);
    } else {
        Q_EMIT m_participants[Myself]->videoStreamRemoved(m_participants[Myself]);
    }

    //remove the participant if it has no other stream on it
    if ((audio && !m_participants[Myself]->hasVideoStream()) ||
        (!audio && !m_participants[Myself]->hasAudioStream()))
    {
        CallParticipant *p = m_participants.take(Myself);
        Q_EMIT q->participantLeft(p);
        p->deleteLater();
    }
}


void CallChannelHandlerPrivate::createAudioBin(QExplicitlySharedDataPointer<ParticipantData> & data)
{
    data->audioBin = QGst::Bin::create();

    data->volumeControl = QGst::ElementFactory::make("volume").dynamicCast<QGst::StreamVolume>();
    data->audioBin->add(data->volumeControl);
    data->audioBin->addPad(QGst::GhostPad::create(data->volumeControl->getStaticPad("sink"), "sink"));

    QGst::ElementPtr audioConvert = QGst::ElementFactory::make("audioconvert");
    data->audioBin->add(audioConvert);
    data->volumeControl->link(audioConvert);

    QGst::ElementPtr audioResample = QGst::ElementFactory::make("audioresample");
    data->audioBin->add(audioResample);
    audioConvert->link(audioResample);

    data->audioBin->addPad(QGst::GhostPad::create(audioResample->getStaticPad("src"), "src"));
}

void CallChannelHandlerPrivate::createVideoBin(QExplicitlySharedDataPointer<ParticipantData> & data, bool withSink)
{
    data->videoBin = QGst::Bin::create();

    data->colorBalanceControl = QGst::ElementFactory::make("videobalance").dynamicCast<QGst::ColorBalance>();
    data->videoBin->add(data->colorBalanceControl);
    data->videoBin->addPad(QGst::GhostPad::create(data->colorBalanceControl->getStaticPad("sink"), "sink"));

    QGst::ElementPtr tee = QGst::ElementFactory::make("tee");
    data->videoBin->add(tee);
    data->colorBalanceControl->link(tee);

    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");
    data->videoBin->add(queue);
    tee->getRequestPad("src%d")->link(queue->getStaticPad("sink"));

    QGst::ElementPtr colorspace = QGst::ElementFactory::make("ffmpegcolorspace");
    data->videoBin->add(colorspace);
    queue->link(colorspace);

    QGst::ElementPtr videoscale = QGst::ElementFactory::make("videoscale");
    data->videoBin->add(videoscale);
    colorspace->link(videoscale);

    QGst::ElementPtr videosink = QGst::ElementFactory::make("qwidgetvideosink"); //FIXME not always the best sink
    videosink->setProperty("force-aspect-ratio", true); //FIXME should be externally configurable
    data->videoBin->add(videosink);
    videoscale->link(videosink);

    data->videoWidget = new QGst::Ui::VideoWidget;
    data->videoWidget.data()->setVideoSink(videosink);

    if (withSink) {
        QGst::ElementPtr queue2 = QGst::ElementFactory::make("queue");
        data->videoBin->add(queue2);
        tee->getRequestPad("src%d")->link(queue2->getStaticPad("sink"));

        data->videoBin->addPad(QGst::GhostPad::create(queue2->getStaticPad("src"), "src"));
    }

//    m_pipeline->add(data->videoBin);
//     if (!sinkPad.isNull()) {
//         m_videoBin->getStaticPad("src")->link(sinkPad);
//     }
//     srcPad->link(m_videoBin->getStaticPad("sink"));
//    data->videoBin->setState(QGst::StatePlaying);
}

#include "callchannelhandler.moc"
#include "callchannelhandler_p.moc"
