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

#include "tf-channel-handler.h"
#include "tf-content-handler.h"
#include "libktpcall_debug.h"

#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Init>
#include <QGst/Bus>

namespace KTpCallPrivate {

TfChannelHandler::TfChannelHandler(const Tp::CallChannelPtr & channel,
                                   TfContentHandlerFactory::Constructor factoryCtor,
                                   QObject *parent)
    : QObject(parent),
      m_callChannel(channel),
      m_factory(factoryCtor()),
      m_channelClosedCounter(1)
{
    connect(m_callChannel.data(), SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onCallChannelInvalidated()));

    QTimer::singleShot(0, this, SLOT(init()));
}

TfChannelHandler::~TfChannelHandler()
{
    delete m_factory;
}

void TfChannelHandler::shutdown()
{
    //This will cause invalidated() to be emited on the 2 proxies (Tp::Channel & TpChannel)
    //we catch the tp-qt one in onCallChannelInvalidated() and the tp-glib one
    //in onTfChannelClosed() through TfChannel, which is closed when the TpChannel
    //is invalidated. When both are invalidated, we emit the channelClosed() signal.
    (void) m_callChannel->requestClose();
}

void TfChannelHandler::init()
{
    try {
        QGst::init();
        QTf::init();
    } catch (const QGlib::Error & error) {
        qCCritical(LIBKTPCALL) << error;
        m_callChannel->hangup(Tp::CallStateChangeReasonInternalError,
                              TP_QT_ERROR_MEDIA_STREAMING_ERROR,
                              error.message());

        return;
    }

    QTf::PendingChannel *pendingChannel = new QTf::PendingChannel(m_callChannel);
    connect(pendingChannel, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onPendingTfChannelFinished(Tp::PendingOperation*)));
}

void TfChannelHandler::onPendingTfChannelFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qCCritical(LIBKTPCALL) << op->errorMessage();
        m_callChannel->hangup(Tp::CallStateChangeReasonInternalError,
                              TP_QT_ERROR_MEDIA_STREAMING_ERROR,
                              op->errorMessage());
        return;
    }

    qCDebug(LIBKTPCALL) << "TfChannel ready";
    m_tfChannel = qobject_cast<QTf::PendingChannel*>(op)->channel();
    m_channelClosedCounter--; // from this point on, we also need to wait for TfChannel to close

    m_pipeline = QGst::Pipeline::create();
    m_pipeline->setState(QGst::StatePlaying);

    m_pipeline->bus()->addSignalWatch();
    QGlib::connect(m_pipeline->bus(), "message", this, &TfChannelHandler::onBusMessage);

    QGlib::connect(m_tfChannel, "closed",
                   this, &TfChannelHandler::onTfChannelClosed);
    QGlib::connect(m_tfChannel, "content-added",
                   this, &TfChannelHandler::onContentAdded);
    QGlib::connect(m_tfChannel, "content-removed",
                   this, &TfChannelHandler::onContentRemoved);
    QGlib::connect(m_tfChannel, "fs-conference-added",
                   this, &TfChannelHandler::onFsConferenceAdded);
    QGlib::connect(m_tfChannel, "fs-conference-removed",
                   this, &TfChannelHandler::onFsConferenceRemoved);
}

void TfChannelHandler::onCallChannelInvalidated()
{
    qCDebug(LIBKTPCALL) << "Tp::Channel invalidated";

    if (++m_channelClosedCounter == 2) {
        qCDebug(LIBKTPCALL) << "emit channelClosed()";
        Q_EMIT channelClosed();
    }
}

void TfChannelHandler::onTfChannelClosed()
{
    qCDebug(LIBKTPCALL) << "TfChannel closed";

    Q_ASSERT(m_pipeline);
    m_pipeline->bus()->removeSignalWatch();
    m_pipeline->setState(QGst::StateNull);
    m_fsElementAddedNotifiers.clear();

    if (++m_channelClosedCounter == 2) {
        qCDebug(LIBKTPCALL) << "emit channelClosed()";
        Q_EMIT channelClosed();
    }
}

void TfChannelHandler::onContentAdded(const QTf::ContentPtr & tfContent)
{
    qCDebug(LIBKTPCALL) << "TfContent added:" << tfContent->property("object-path").toString();

    TfContentHandler *contentHandler = m_factory->createContentHandler(tfContent, this);

    m_contents.insert(tfContent, contentHandler);
    Q_EMIT contentAdded(contentHandler);
}

void TfChannelHandler::onContentRemoved(const QTf::ContentPtr & tfContent)
{
    qCDebug(LIBKTPCALL) << "TfContent removed:" << tfContent->property("object-path").toString();

    TfContentHandler *contentHandler = m_contents.take(tfContent);
    contentHandler->cleanup();
    Q_EMIT contentRemoved(contentHandler);
    delete contentHandler;
}

void TfChannelHandler::onFsConferenceAdded(const QGst::ElementPtr & conference)
{
    qCDebug(LIBKTPCALL) << "Adding fsconference in the pipeline";
    m_fsElementAddedNotifiers.append(QTf::loadFsElementAddedNotifier(conference, m_pipeline));
    m_pipeline->add(conference);
    conference->syncStateWithParent();
}

void TfChannelHandler::onFsConferenceRemoved(const QGst::ElementPtr & conference)
{
    qCDebug(LIBKTPCALL) << "Removing fsconference from the pipeline";
    m_pipeline->remove(conference);
    conference->setState(QGst::StateNull);
}

void TfChannelHandler::onBusMessage(const QGst::MessagePtr & message)
{
    m_tfChannel->processBusMessage(message);
}

} // KTpCallPrivate
