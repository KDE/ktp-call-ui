/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
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
#include "call-content-handler_p.h"
#include <TelepathyQt/ReferencedHandles>
#include <QGlib/Connect>
#include <QGst/Pad>
#include <QGst/ElementFactory>
#include <KDebug>

//BEGIN PendingCallContentHandler

PendingCallContentHandler::PendingCallContentHandler(const Tp::CallChannelPtr & callChannel,
                                                     const QTf::ContentPtr & tfContent,
                                                     const QGst::PipelinePtr & pipeline,
                                                     QObject *parent)
    : QObject(parent)
{
    switch(tfContent->property("media-type").toInt()) {
    case Tp::MediaStreamTypeAudio:
        m_contentHandler = new AudioContentHandler(parent);
        break;
    case Tp::MediaStreamTypeVideo:
        m_contentHandler = new VideoContentHandler(parent);
        break;
    default:
        Q_ASSERT(false);
    }

    m_contentHandler->d->init(tfContent, pipeline);

    m_callChannel = callChannel;
    m_tfContent = tfContent;
    QTimer::singleShot(0, this, SLOT(findCallContent()));
}

void PendingCallContentHandler::findCallContent()
{
    Tp::CallContents callContents = m_callChannel->contents();
    Q_FOREACH (const Tp::CallContentPtr & callContent, callContents) {
        if (callContent->objectPath() == m_tfContent->property("object-path").toString()) {
            m_contentHandler->d->setCallContent(callContent);
            Q_EMIT ready(m_tfContent, m_contentHandler);
            deleteLater();
            return;
        }
    }

    kDebug() << "CallContent not found. Waiting for tp-qt4 to synchronize with d-bus.";
    connect(m_callChannel.data(), SIGNAL(contentAdded(Tp::CallContentPtr)),
            SLOT(onContentAdded(Tp::CallContentPtr)));
}

void PendingCallContentHandler::onContentAdded(const Tp::CallContentPtr & callContent)
{
    if (callContent->objectPath() == m_tfContent->property("object-path").toString()) {
        m_contentHandler->d->setCallContent(callContent);
        Q_EMIT ready(m_tfContent, m_contentHandler);
        deleteLater();
    }
}

//END PendingCallContentHandler
//BEGIN CallContentHandlerPrivate

CallContentHandlerPrivate::~CallContentHandlerPrivate()
{
    kDebug();
    m_sourceControllerPad->unlink(m_queue->getStaticPad("sink"));
    if (m_tfContent->property("sink-pad").get<QGst::PadPtr>()) {
        m_queue->getStaticPad("src")->unlink(m_tfContent->property("sink-pad").get<QGst::PadPtr>());
    }
    m_queue->setState(QGst::StateNull);
    m_sourceController->releaseSrcPad(m_sourceControllerPad);
    m_sinkManager->unlinkAllPads();

    delete m_sourceController;
    delete m_sinkManager;
}

void CallContentHandlerPrivate::init(const QTf::ContentPtr & tfContent,
                                     const QGst::PipelinePtr & pipeline)
{
    kDebug();

    m_tfContent = tfContent;
    m_pipeline = pipeline;
    QGlib::connect(tfContent, "src-pad-added", this, &CallContentHandlerPrivate::onSrcPadAdded);
    QGlib::connect(tfContent, "start-sending", this, &CallContentHandlerPrivate::onStartSending);
    QGlib::connect(tfContent, "stop-sending", this, &CallContentHandlerPrivate::onStopSending);
    QGlib::connect(tfContent, "start-receiving", this, &CallContentHandlerPrivate::onStartReceiving);
    QGlib::connect(tfContent, "stop-receiving", this, &CallContentHandlerPrivate::onStopReceiving);

    switch(tfContent->property("media-type").toInt()) {
    case Tp::MediaStreamTypeAudio:
        m_sourceController = new AudioSourceController(pipeline);
        m_sinkManager = new AudioSinkManager(pipeline, this);
        break;
    case Tp::MediaStreamTypeVideo:
        m_sourceController = new VideoSourceController(pipeline);
        m_sinkManager = new VideoSinkManager(pipeline, this);
        break;
    default:
        Q_ASSERT(false);
    }

    m_queue = QGst::ElementFactory::make("queue");
    m_pipeline->add(m_queue);
    m_queue->syncStateWithParent();

    m_sourceControllerPad = m_sourceController->requestSrcPad();
    m_sourceControllerPad->link(m_queue->getStaticPad("sink"));
    m_queue->getStaticPad("src")->link(m_tfContent->property("sink-pad").get<QGst::PadPtr>());

    connect(m_sinkManager, SIGNAL(controllerCreated(BaseSinkController*)),
            this, SLOT(onControllerCreated(BaseSinkController*)));
    connect(m_sinkManager, SIGNAL(controllerDestroyed(BaseSinkController*)),
            this, SLOT(onControllerDestroyed(BaseSinkController*)));
}

void CallContentHandlerPrivate::setCallContent(const Tp::CallContentPtr & callContent)
{
    kDebug();
    m_callContent = callContent;

    //the sink manager needs the content to discover contacts
    m_sinkManager->setCallContent(callContent);
}

void CallContentHandlerPrivate::onSrcPadAdded(uint contactHandle,
                                              const QGlib::ObjectPtr & fsStream,
                                              const QGst::PadPtr & pad)
{
    Q_UNUSED(fsStream);
    m_sinkManager->handleNewSinkPad(contactHandle, pad);
}

bool CallContentHandlerPrivate::onStartSending()
{
    kDebug() << "Start sending requested";

    m_sourceController->connectToSource();
    Q_EMIT q->localSendingStateChanged(true);
    return true;
}

void CallContentHandlerPrivate::onStopSending()
{
    kDebug() << "Stop sending requested";

    m_sourceController->disconnectFromSource();
    Q_EMIT q->localSendingStateChanged(false);
}

bool CallContentHandlerPrivate::onStartReceiving(void *handles, uint handleCount)
{
    kDebug() << "Start receiving requested";

    uint *ihandles = reinterpret_cast<uint*>(handles);
    for (uint i = 0; i < handleCount; ++i) {
        if (m_handlesToContacts.contains(ihandles[i])) {
            Tp::ContactPtr contact = m_handlesToContacts[ihandles[i]];
            Q_ASSERT(m_sinkControllers.contains(contact));

            //if we are not already receiving
            if (!m_sinkControllers[contact].second) {
                kDebug() << "Starting receiving from" << contact->alias();

                m_sinkControllers[contact].second = true;
                Q_EMIT q->remoteSendingStateChanged(contact, true);
            }
        }
    }
    return true;
}

void CallContentHandlerPrivate::onStopReceiving(void *handles, uint handleCount)
{
    kDebug() << "Stop receiving requested";

    uint *ihandles = reinterpret_cast<uint*>(handles);
    for (uint i = 0; i < handleCount; ++i) {
        if (m_handlesToContacts.contains(ihandles[i])) {
            Tp::ContactPtr contact = m_handlesToContacts[ihandles[i]];
            Q_ASSERT(m_sinkControllers.contains(contact));

            //if we are receiving
            if (m_sinkControllers[contact].second) {
                kDebug() << "Stopping receiving from" << contact->alias();

                m_sinkControllers[contact].second = false;
                Q_EMIT q->remoteSendingStateChanged(contact, false);
            }
        }
    }
}

void CallContentHandlerPrivate::onControllerCreated(BaseSinkController *controller)
{
    kDebug() << "Sink controller created. Contact:" << controller->contact()->alias()
             << "media-type:" << m_tfContent->property("media-type").toInt();

    m_sinkControllers.insert(controller->contact(), qMakePair(controller, true));
    m_handlesToContacts.insert(controller->contact()->handle()[0], controller->contact());

    Q_EMIT q->remoteSendingStateChanged(controller->contact(), true);
}

void CallContentHandlerPrivate::onControllerDestroyed(BaseSinkController *controller)
{
    kDebug() << "Sink controller destroyed. Contact:" << controller->contact()->alias()
             << "media-type:" << m_tfContent->property("media-type").toInt();

    bool wasReceiving = m_sinkControllers.take(controller->contact()).second;
    m_handlesToContacts.remove(controller->contact()->handle()[0]);

    if (wasReceiving) {
        Q_EMIT q->remoteSendingStateChanged(controller->contact(), false);
    }
}

//END CallContentHandlerPrivate
//BEGIN CallContentHandler

CallContentHandler::CallContentHandler(QObject *parent)
    : QObject(parent), d(new CallContentHandlerPrivate(this))
{
}

CallContentHandler::~CallContentHandler()
{
    delete d;
}

Tp::CallContentPtr CallContentHandler::callContent() const
{
    return d->m_callContent;
}

Tp::Contacts CallContentHandler::remoteMembers() const
{
    return d->m_sinkControllers.keys().toSet();
}

//END CallContentHandler
//BEGIN AudioContentHandler

AudioContentHandler::AudioContentHandler(QObject *parent)
    : CallContentHandler(parent)
{
}

VolumeController *AudioContentHandler::sourceVolumeControl() const
{
    return static_cast<AudioSourceController*>(d->m_sourceController)->volumeController();
}

VolumeController *AudioContentHandler::remoteMemberVolumeControl(const Tp::ContactPtr & contact) const
{
    if (d->m_sinkControllers.contains(contact)) {
        BaseSinkController *ctrl = d->m_sinkControllers[contact].first;
        return static_cast<AudioSinkController*>(ctrl)->volumeController();
    } else {
        return NULL;
    }
}

//END AudioContentHandler
//BEGIN VideoContentHandler

VideoContentHandler::VideoContentHandler(QObject *parent)
    : CallContentHandler(parent)
{
}

void VideoContentHandler::linkVideoPreviewSink(const QGst::ElementPtr & sink)
{
    static_cast<VideoSourceController*>(d->m_sourceController)->linkVideoPreviewSink(sink);
}

void VideoContentHandler::unlinkVideoPreviewSink()
{
    static_cast<VideoSourceController*>(d->m_sourceController)->unlinkVideoPreviewSink();
}

void VideoContentHandler::linkRemoteMemberVideoSink(const Tp::ContactPtr & contact,
                                                    const QGst::ElementPtr & sink)
{
    if (d->m_sinkControllers.contains(contact)) {
        BaseSinkController *ctrl = d->m_sinkControllers[contact].first;
        static_cast<VideoSinkController*>(ctrl)->linkVideoSink(sink);
    }
}

void VideoContentHandler::unlinkRemoteMemberVideoSink(const Tp::ContactPtr & contact)
{
    if (d->m_sinkControllers.contains(contact)) {
        BaseSinkController *ctrl = d->m_sinkControllers[contact].first;
        static_cast<VideoSinkController*>(ctrl)->unlinkVideoSink();
    }
}

//END VideoContentHandler

#include "moc_call-content-handler.cpp"
#include "moc_call-content-handler_p.cpp"
