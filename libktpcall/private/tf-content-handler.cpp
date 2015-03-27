/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
    Copyright (C) 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tf-content-handler.h"
#include "sink-manager.h"
#include "libktpcall_debug.h"

#include <QGlib/Connect>
#include <TelepathyQt/ReferencedHandles>

namespace KTpCallPrivate {

TfContentHandler::TfContentHandler(const QTf::ContentPtr & tfContent, TfChannelHandler *parent)
    : QObject(parent),
      m_tfContent(tfContent),
      m_sending(false)
{
    qCDebug(LIBKTPCALL);

    QGlib::connect(m_tfContent, "src-pad-added", this, &TfContentHandler::onSrcPadAdded);
    QGlib::connect(m_tfContent, "start-sending", this, &TfContentHandler::onStartSending);
    QGlib::connect(m_tfContent, "stop-sending", this, &TfContentHandler::onStopSending);
    QGlib::connect(m_tfContent, "start-receiving", this, &TfContentHandler::onStartReceiving);
    QGlib::connect(m_tfContent, "stop-receiving", this, &TfContentHandler::onStopReceiving);

    m_sinkManager = new SinkManager(this);
    connect(m_sinkManager, SIGNAL(controllerCreated(KTpCallPrivate::BaseSinkController*)),
            this, SLOT(onControllerCreated(KTpCallPrivate::BaseSinkController*)));
    connect(m_sinkManager, SIGNAL(controllerDestroyed(KTpCallPrivate::BaseSinkController*)),
            this, SLOT(onControllerDestroyed(KTpCallPrivate::BaseSinkController*)));

    QTimer::singleShot(0, this, SLOT(findCallContent()));
}

TfContentHandler::~TfContentHandler()
{
    qCDebug(LIBKTPCALL);
    delete m_sinkManager;
}

Tp::Contacts TfContentHandler::remoteMembers() const
{
    return m_sinkControllers.keys().toSet();
}

BaseSinkController *TfContentHandler::sinkController(const Tp::ContactPtr& contact) const
{
    return m_sinkControllers.contains(contact) ? m_sinkControllers[contact].first : 0;
}

void TfContentHandler::cleanup()
{
    qCDebug(LIBKTPCALL);
    if (m_sending) {
        qCDebug(LIBKTPCALL) << "Cleanup detected we are still sending - stopping sending";
        stopSending();
        m_sending = false;
        Q_EMIT localSendingStateChanged(false);
    }
    m_sinkManager->cleanup();
}

void TfContentHandler::onSrcPadAdded(uint contactHandle,
                                     const QGlib::ObjectPtr & fsStream,
                                     const QGst::PadPtr & pad)
{
    Q_UNUSED(fsStream);
    m_sinkManager->handleNewSinkPad(contactHandle, pad);
}

bool TfContentHandler::onStartSending()
{
    qCDebug(LIBKTPCALL) << "Start sending requested";

    bool success = startSending();
    if (success) {
        qCDebug(LIBKTPCALL) << "Started sending successfully";
        m_sending = true;
        Q_EMIT localSendingStateChanged(true);
    }
    return success;
}

void TfContentHandler::onStopSending()
{
    qCDebug(LIBKTPCALL) << "Stop sending requested";

    if (m_sending) {
        qCDebug(LIBKTPCALL) << "Stopping sending";
        stopSending();
        m_sending = false;
        Q_EMIT localSendingStateChanged(false);
    }
}

bool TfContentHandler::onStartReceiving(void *handles, uint handleCount)
{
    qCDebug(LIBKTPCALL) << "Start receiving requested";

    uint *ihandles = reinterpret_cast<uint*>(handles);
    for (uint i = 0; i < handleCount; ++i) {
        if (m_handlesToContacts.contains(ihandles[i])) {
            Tp::ContactPtr contact = m_handlesToContacts[ihandles[i]];
            Q_ASSERT(m_sinkControllers.contains(contact));

            //if we are not already receiving
            if (!m_sinkControllers[contact].second) {
                qCDebug(LIBKTPCALL) << "Starting receiving from" << contact->id();

                m_sinkControllers[contact].second = true;
                Q_EMIT remoteSendingStateChanged(contact, true);
            }
        }
    }
    return true;
}

void TfContentHandler::onStopReceiving(void *handles, uint handleCount)
{
    qCDebug(LIBKTPCALL) << "Stop receiving requested";

    uint *ihandles = reinterpret_cast<uint*>(handles);
    for (uint i = 0; i < handleCount; ++i) {
        if (m_handlesToContacts.contains(ihandles[i])) {
            Tp::ContactPtr contact = m_handlesToContacts[ihandles[i]];
            Q_ASSERT(m_sinkControllers.contains(contact));

            //if we are receiving
            if (m_sinkControllers[contact].second) {
                qCDebug(LIBKTPCALL) << "Stopping receiving from" << contact->id();

                m_sinkControllers[contact].second = false;
                Q_EMIT remoteSendingStateChanged(contact, false);
            }
        }
    }
}

void TfContentHandler::onControllerCreated(BaseSinkController *controller)
{
    qCDebug(LIBKTPCALL) << "Sink controller created. Contact:" << controller->contact()->id()
             << "media-type:" << m_tfContent->property("media-type").toInt();

    m_sinkControllers.insert(controller->contact(), qMakePair(controller, true));
    m_handlesToContacts.insert(controller->contact()->handle()[0], controller->contact());

    Q_EMIT remoteSendingStateChanged(controller->contact(), true);
}

void TfContentHandler::onControllerDestroyed(BaseSinkController *controller)
{
    qCDebug(LIBKTPCALL) << "Sink controller destroyed. Contact:" << controller->contact()->id()
             << "media-type:" << m_tfContent->property("media-type").toInt();

    bool wasReceiving = m_sinkControllers.take(controller->contact()).second;
    m_handlesToContacts.remove(controller->contact()->handle()[0]);

    if (wasReceiving) {
        Q_EMIT remoteSendingStateChanged(controller->contact(), false);
    }
}

void TfContentHandler::findCallContent()
{
    Tp::CallContents callContents = channelHandler()->callChannel()->contents();
    Q_FOREACH (const Tp::CallContentPtr & callContent, callContents) {
        if (callContent->objectPath() == m_tfContent->property("object-path").toString()) {
            m_callContent = callContent;
            Q_EMIT callContentReady(this);
            return;
        }
    }

    // Because tp-qt and tp-glib check dbus separately, tp-qt may not be synchronized
    // with tp-glib and therefore the Tp::CallContent may not be available when the
    // TfContent appears. Here, we wait for tp-qt to synchronize.

    qCDebug(LIBKTPCALL) << "CallContent not found. Waiting for tp-qt to synchronize with d-bus.";
    connect(channelHandler()->callChannel().data(),
            SIGNAL(contentAdded(Tp::CallContentPtr)),
            SLOT(onContentAdded(Tp::CallContentPtr)));
}

void TfContentHandler::onContentAdded(const Tp::CallContentPtr & callContent)
{
    if (callContent->objectPath() == m_tfContent->property("object-path").toString()) {
        m_callContent = callContent;
        Q_EMIT callContentReady(this);

        disconnect(channelHandler()->callChannel().data(),
                   SIGNAL(contentAdded(Tp::CallContentPtr)),
                   this, SLOT(onContentAdded(Tp::CallContentPtr)));
    }
}

} // KTpCallPrivate
