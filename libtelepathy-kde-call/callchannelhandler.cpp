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
#include "callparticipant_p.h"
#include "deviceelementfactory_p.h"
#include "../libqtpfarsight/qtfchannel.h"
#include <QGst/ElementFactory>
#include <QGst/Bus>
#include <KDebug>
#include <KLocalizedString>
#include <KStandardDirs>
#include <TelepathyQt4/Connection>

CallChannelHandler::CallChannelHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent)
    : QObject(parent), d(new CallChannelHandlerPrivate(this))
{
    d->init(channel);
}

CallChannelHandler::~CallChannelHandler()
{
    delete d;
}

QList<CallParticipant*> CallChannelHandler::participants() const
{
    return d->participants().values();
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

    m_qtfchannel = new QTfChannel(channel, m_pipeline->bus(), this);
    connect(m_qtfchannel, SIGNAL(sessionCreated(QGst::ElementPtr)),
            SLOT(onSessionCreated(QGst::ElementPtr)));
    connect(m_qtfchannel, SIGNAL(closed()), SLOT(onQTfChannelClosed()));

    connect(m_qtfchannel, SIGNAL(openAudioInputDevice(bool*)), SLOT(openAudioInputDevice(bool*)));
    connect(m_qtfchannel, SIGNAL(audioSinkPadAdded(QGst::PadPtr)),
            SLOT(audioSinkPadAdded(QGst::PadPtr)));
    connect(m_qtfchannel, SIGNAL(closeAudioInputDevice()), SLOT(closeAudioInputDevice()));

    connect(m_qtfchannel, SIGNAL(openVideoInputDevice(bool*)), SLOT(openVideoInputDevice(bool*)));
    connect(m_qtfchannel, SIGNAL(videoSinkPadAdded(QGst::PadPtr)),
            SLOT(videoSinkPadAdded(QGst::PadPtr)));
    connect(m_qtfchannel, SIGNAL(closeVideoInputDevice()), SLOT(closeVideoInputDevice()));

    connect(m_qtfchannel, SIGNAL(openAudioOutputDevice(bool*)), SLOT(openAudioOutputDevice(bool*)));
    connect(m_qtfchannel, SIGNAL(audioSrcPadAdded(QGst::PadPtr)),
            SLOT(audioSrcPadAdded(QGst::PadPtr)));
    connect(m_qtfchannel, SIGNAL(audioSrcPadRemoved(QGst::PadPtr)),
            SLOT(audioSrcPadRemoved(QGst::PadPtr)));

    connect(m_qtfchannel, SIGNAL(openVideoOutputDevice(bool*)), SLOT(openVideoOutputDevice(bool*)));
    connect(m_qtfchannel, SIGNAL(videoSrcPadAdded(QGst::PadPtr)),
            SLOT(videoSrcPadAdded(QGst::PadPtr)));
    connect(m_qtfchannel, SIGNAL(videoSrcPadRemoved(QGst::PadPtr)),
            SLOT(videoSrcPadRemoved(QGst::PadPtr)));

    QString codecsFile = KStandardDirs::locate("data", "libtelepathy-kde-call/codec-preferences.ini");
    if ( !codecsFile.isEmpty() ) {
        kDebug() << "Reading codec preferences from" << codecsFile;
        m_qtfchannel->setCodecsConfigFile(codecsFile);
    }
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

    QGst::ElementPtr element = DeviceElementFactory::makeAudioCaptureElement();
    if ( !element ) {
        Q_EMIT q->error(i18n("The audio input device could not be initialized"));
        *success = false;
        return;
    }

    m_audioInputDevice = element;
    *success = true;
}

void CallChannelHandlerPrivate::audioSinkPadAdded(QGst::PadPtr sinkPad)
{
    kDebug() << "Audio sink pad added";

    Q_ASSERT(!m_audioInputDevice.isNull());

    //create the participant if not already there
    Tp::ContactPtr contact = m_channel->connection()->selfContact();
    bool joined = false;
    if (!m_participants.contains(contact)) {
        m_participants[contact] = new CallParticipant(contact, q);
        joined = true;
    }

    //add the device in the pipeline
    QGst::State currentState;
    m_audioInputDevice->getState(&currentState, 0, 0);
    if (currentState == QGst::StateReady) {
        m_pipeline->add(m_audioInputDevice);
    }

    //add the pads to the participant
    QGst::PadPtr srcPad = m_audioInputDevice->getStaticPad("src");
    m_participants[contact]->d->setAudioPads(m_pipeline, srcPad, sinkPad);

    //set the input device to playing state
    if (currentState == QGst::StateReady) {
        m_audioInputDevice->setState(QGst::StatePlaying);
    }

    //if we just created the participant, emit the joined signal
    if (joined) {
        Q_EMIT q->participantJoined(m_participants[contact]);
    }
}

void CallChannelHandlerPrivate::closeAudioInputDevice()
{
    kDebug() << "Closing audio input device";

    //remove participant's audio pipeline
    Tp::ContactPtr contact = m_channel->connection()->selfContact();
    m_participants[contact]->d->removeAudioPads(m_pipeline);

    //destroy the audio input device
    m_audioInputDevice->setState(QGst::StateNull);
    m_pipeline->remove(m_audioInputDevice);
    m_audioInputDevice = QGst::ElementPtr();

    //remove the participant if it has no other stream on it
    if (!m_participants[contact]->hasVideoStream()) {
        CallParticipant *p = m_participants.take(contact);
        Q_EMIT q->participantLeft(p);
        p->deleteLater();
    }
}

void CallChannelHandlerPrivate::openVideoInputDevice(bool *success)
{
    kDebug() << "Opening video input device";

    QGst::ElementPtr element = DeviceElementFactory::makeVideoCaptureElement();
    if ( !element ) {
        Q_EMIT q->error(i18n("The video input device could not be initialized."));
        *success = false;
        return;
    }

    m_videoInputDevice = element;
    *success = true;
}

void CallChannelHandlerPrivate::videoSinkPadAdded(QGst::PadPtr sinkPad)
{
    kDebug() << "Video sink pad added";

    Q_ASSERT(!m_videoInputDevice.isNull());

    //create the participant if not already there
    Tp::ContactPtr contact = m_channel->connection()->selfContact();
    bool joined = false;
    if (!m_participants.contains(contact)) {
        m_participants[contact] = new CallParticipant(contact, q);
        joined = true;
    }

    //add the device in the pipeline
    QGst::State currentState;
    m_videoInputDevice->getState(&currentState, 0, 0);
    if (currentState == QGst::StateReady) {
        m_pipeline->add(m_videoInputDevice);
    }

    //add the pads to the participant
    QGst::PadPtr srcPad = m_videoInputDevice->getStaticPad("src");
    m_participants[contact]->d->setVideoPads(m_pipeline, srcPad, sinkPad);

    //set the input device to playing state
    if (currentState == QGst::StateReady) {
        m_videoInputDevice->setState(QGst::StatePlaying);
    }

    //if we just created the participant, emit the joined signal
    if (joined) {
        Q_EMIT q->participantJoined(m_participants[contact]);
    }
}

void CallChannelHandlerPrivate::closeVideoInputDevice()
{
    kDebug() << "Closing video input device";

    //remove participant's video pipeline
    Tp::ContactPtr contact = m_channel->connection()->selfContact();
    m_participants[contact]->d->removeVideoPads(m_pipeline);

    //destroy the video input device
    m_videoInputDevice->setState(QGst::StateNull);
    m_pipeline->remove(m_videoInputDevice);
    m_videoInputDevice = QGst::ElementPtr();

    //remove the participant if it has no other stream on it
    if (!m_participants[contact]->hasAudioStream()) {
        CallParticipant *p = m_participants.take(contact);
        Q_EMIT q->participantLeft(p);
        p->deleteLater();
    }
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

    m_audioOutputAdder = QGst::ElementFactory::make("liveadder");
    if ( !m_audioOutputAdder ) {
        kDebug() << "Using adder instead of liveadder";
        m_audioOutputAdder = QGst::ElementFactory::make("adder");
    }

    if ( !m_audioOutputAdder ) {
        Q_EMIT q->error(i18n("Some gstreamer elements could not be created. "
                             "Please check your gstreamer installation."));
        *success = false;
        return;
    }

    m_audioOutputDevice = element;
    m_audioOutputAdder->setState(QGst::StateReady);
    m_audioOutputAdder->link(m_audioOutputDevice);
    *success = true;
}

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

void CallChannelHandlerPrivate::audioSrcPadAdded(QGst::PadPtr srcPad)
{
    kDebug() << "Audio src pad added";

    Q_ASSERT(!m_audioOutputAdder.isNull() && !m_audioOutputDevice.isNull());

    //set the device to playing state
    QGst::State currentState;
    m_audioOutputDevice->getState(&currentState, 0, 0);
    if (currentState == QGst::StateReady) {
        m_pipeline->add(m_audioOutputAdder);
        m_pipeline->add(m_audioOutputDevice);
        m_audioOutputAdder->setState(QGst::StatePlaying);
        m_audioOutputDevice->setState(QGst::StatePlaying);
    }

    //create the participant if not already there
    Tp::ContactPtr contact = findRemoteContact(m_channel);
    bool joined = false;
    if (!m_participants.contains(contact)) {
        m_participants[contact] = new CallParticipant(contact, q);
        joined = true;
    }

    //get a sink pad from the adder
    QGst::PadPtr sinkPad = m_audioOutputAdder->getRequestPad("sink%d");
    m_audioAdderPadsMap.insert(srcPad->property("name").get<QByteArray>(), sinkPad);

    //add the pads to the participant
    m_participants[contact]->d->setAudioPads(m_pipeline, srcPad, sinkPad);

    //if we just created the participant, emit the joined signal
    if (joined) {
        Q_EMIT q->participantJoined(m_participants[contact]);
    }
}

void CallChannelHandlerPrivate::audioSrcPadRemoved(QGst::PadPtr srcPad)
{
    kDebug() << "Audio src pad removed";

    //remove participant's audio pipeline
    Tp::ContactPtr contact = findRemoteContact(m_channel);
    m_participants[contact]->d->removeAudioPads(m_pipeline);

    //release the pad from the adder
    QGst::PadPtr sinkPad = m_audioAdderPadsMap.take(srcPad->property("name").get<QByteArray>());
    m_audioOutputAdder->releaseRequestPad(sinkPad);

    //remove the audio output device if nothing is connected to the adder anymore
    if (m_audioAdderPadsMap.isEmpty()) {
        kDebug() << "Shutting down audio output device";
        m_audioOutputAdder->setState(QGst::StateReady);
        m_audioOutputDevice->setState(QGst::StateReady);
        m_pipeline->remove(m_audioOutputAdder);
        m_pipeline->remove(m_audioOutputDevice);
    }

    //remove the participant if it has no other stream on it
    if (!m_participants[contact]->hasVideoStream()) {
        CallParticipant *p = m_participants.take(contact);
        Q_EMIT q->participantLeft(p);
        p->deleteLater();
    }
}

void CallChannelHandlerPrivate::openVideoOutputDevice(bool *success)
{
    kDebug() << "Opening video output device";
    //succeed if we have qwidgetvideosink installed
    *success = QGst::ElementFactory::find("qwidgetvideosink");
}

void CallChannelHandlerPrivate::videoSrcPadAdded(QGst::PadPtr srcPad)
{
    kDebug() << "Video src pad added";

    //create the participant if not already there
    Tp::ContactPtr contact = findRemoteContact(m_channel);
    bool joined = false;
    if (!m_participants.contains(contact)) {
        m_participants[contact] = new CallParticipant(contact, q);
        joined = true;
    }

    //add the pads to the participant
    m_participants[contact]->d->setVideoPads(m_pipeline, srcPad);

    //if we just created the participant, emit the joined signal
    if (joined) {
        Q_EMIT q->participantJoined(m_participants[contact]);
    }
}

void CallChannelHandlerPrivate::videoSrcPadRemoved(QGst::PadPtr srcPad)
{
    Q_UNUSED(srcPad);

    kDebug() << "Video src pad removed";

    //remove participant's video pipeline
    Tp::ContactPtr contact = findRemoteContact(m_channel);
    m_participants[contact]->d->removeVideoPads(m_pipeline);

    //remove the participant if it has no other stream on it
    if (!m_participants[contact]->hasAudioStream()) {
        CallParticipant *p = m_participants.take(contact);
        Q_EMIT q->participantLeft(p);
        p->deleteLater();
    }
}

#include "callchannelhandler.moc"
#include "callchannelhandler_p.moc"
