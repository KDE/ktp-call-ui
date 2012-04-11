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
#include "call-channel-handler.h"
#include "call-content-handler_p.h"
#include "../libqtf/qtf.h"
#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Init>
#include <QGst/Bus>
#include <KDebug>

//BEGIN CallChannelHandlerPrivate

class CallChannelHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    CallChannelHandlerPrivate(const Tp::CallChannelPtr & channel, CallChannelHandler *qq);

private Q_SLOTS:
    void init();
    void onPendingTfChannelFinished(Tp::PendingOperation *op);
    void onCallContentHandlerReady(const QTf::ContentPtr & tfContent,
                                   CallContentHandler *contentHandler);
    void onCallChannelInvalidated();

private:
    void onTfChannelClosed();
    void onContentAdded(const QTf::ContentPtr & tfContent);
    void onContentRemoved(const QTf::ContentPtr & tfContent);
    void onFsConferenceAdded(const QGst::ElementPtr & conference);
    void onFsConferenceRemoved(const QGst::ElementPtr & conference);
    void onBusMessage(const QGst::MessagePtr & message);

private:
    CallChannelHandler * const q;

    QGst::PipelinePtr m_pipeline;
    QTf::ChannelPtr m_tfChannel;
    uint m_channelClosedCounter;
    QList<QGlib::ObjectPtr> m_fsElementAddedNotifiers;

public:
    Tp::CallChannelPtr m_callChannel;
    QHash<QTf::ContentPtr, CallContentHandler*> m_contents;
};

CallChannelHandlerPrivate::CallChannelHandlerPrivate(const Tp::CallChannelPtr & channel,
                                                     CallChannelHandler *qq)
    : QObject(),
      q(qq),
      m_channelClosedCounter(0),
      m_callChannel(channel)
{
    connect(m_callChannel.data(), SIGNAL(invalidated (Tp::DBusProxy*,QString,QString)),
            SLOT(onCallChannelInvalidated()));

    QTimer::singleShot(0, this, SLOT(init()));
}

void CallChannelHandlerPrivate::init()
{
    try {
        QGst::init();
        QTf::init();
    } catch (const QGlib::Error & error) {
        kError() << error;
        m_callChannel->hangup(Tp::CallStateChangeReasonInternalError,
                              TP_QT_ERROR_MEDIA_STREAMING_ERROR,
                              error.message());

        return;
    }

    QTf::PendingChannel *pendingChannel = new QTf::PendingChannel(m_callChannel);
    connect(pendingChannel, SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onPendingTfChannelFinished(Tp::PendingOperation*)));
}

void CallChannelHandlerPrivate::onPendingTfChannelFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        kError() << op->errorMessage();
        m_callChannel->hangup(Tp::CallStateChangeReasonInternalError,
                              TP_QT_ERROR_MEDIA_STREAMING_ERROR,
                              op->errorMessage());
        return;
    }

    kDebug() << "TfChannel ready";
    m_tfChannel = qobject_cast<QTf::PendingChannel*>(op)->channel();

    m_pipeline = QGst::Pipeline::create();
    m_pipeline->setState(QGst::StatePlaying);

    m_pipeline->bus()->addSignalWatch();
    QGlib::connect(m_pipeline->bus(), "message", this, &CallChannelHandlerPrivate::onBusMessage);

    QGlib::connect(m_tfChannel, "closed",
                   this, &CallChannelHandlerPrivate::onTfChannelClosed);
    QGlib::connect(m_tfChannel, "content-added",
                   this, &CallChannelHandlerPrivate::onContentAdded);
    QGlib::connect(m_tfChannel, "content-removed",
                   this, &CallChannelHandlerPrivate::onContentRemoved);
    QGlib::connect(m_tfChannel, "fs-conference-added",
                   this, &CallChannelHandlerPrivate::onFsConferenceAdded);
    QGlib::connect(m_tfChannel, "fs-conference-removed",
                   this, &CallChannelHandlerPrivate::onFsConferenceRemoved);
}

void CallChannelHandlerPrivate::onCallChannelInvalidated()
{
    kDebug() << "Tp::Channel invalidated";

    if (++m_channelClosedCounter == 2) {
        kDebug() << "emit channelClosed()";
        Q_EMIT q->channelClosed();
    }
}

void CallChannelHandlerPrivate::onTfChannelClosed()
{
    kDebug() << "TfChannel closed";

    Q_ASSERT(m_pipeline);
    m_pipeline->bus()->removeSignalWatch();
    m_pipeline->setState(QGst::StateNull);
    m_fsElementAddedNotifiers.clear();

    if (++m_channelClosedCounter == 2) {
        kDebug() << "emit channelClosed()";
        Q_EMIT q->channelClosed();
    }
}

void CallChannelHandlerPrivate::onContentAdded(const QTf::ContentPtr & tfContent)
{
    kDebug() << "TfContent added. media-type:" << tfContent->property("media-type").toInt();

    connect(new PendingCallContentHandler(m_callChannel, tfContent, m_pipeline, q),
            SIGNAL(ready(QTf::ContentPtr,CallContentHandler*)),
            SLOT(onCallContentHandlerReady(QTf::ContentPtr,CallContentHandler*)));
}

void CallChannelHandlerPrivate::onCallContentHandlerReady(const QTf::ContentPtr & tfContent,
                                                          CallContentHandler *contentHandler)
{
    kDebug() << "CallContentHandler ready";
    m_contents.insert(tfContent, contentHandler);
    Q_EMIT q->contentAdded(contentHandler);
}

void CallChannelHandlerPrivate::onContentRemoved(const QTf::ContentPtr & tfContent)
{
    kDebug() << "TfContent removed. media-type:" << tfContent->property("media-type").toInt();

    CallContentHandler *contentHandler = m_contents.take(tfContent);
    Q_EMIT q->contentRemoved(contentHandler);
    delete contentHandler;
}

void CallChannelHandlerPrivate::onFsConferenceAdded(const QGst::ElementPtr & conference)
{
    kDebug() << "Adding fsconference in the pipeline";
    m_fsElementAddedNotifiers.append(QTf::loadFsElementAddedNotifier(conference, m_pipeline));
    m_pipeline->add(conference);
    conference->syncStateWithParent();
}

void CallChannelHandlerPrivate::onFsConferenceRemoved(const QGst::ElementPtr & conference)
{
    kDebug() << "Removing fsconference from the pipeline";
    m_pipeline->remove(conference);
    conference->setState(QGst::StateNull);
}

void CallChannelHandlerPrivate::onBusMessage(const QGst::MessagePtr & message)
{
    m_tfChannel->processBusMessage(message);
}

//END CallChannelHandlerPrivate
//BEGIN CallChannelHandler

CallChannelHandler::CallChannelHandler(const Tp::CallChannelPtr & channel, QObject *parent)
    : QObject(parent), d(new CallChannelHandlerPrivate(channel, this))
{
}

CallChannelHandler::~CallChannelHandler()
{
    delete d;
}

QList<CallContentHandler*> CallChannelHandler::contents() const
{
    return d->m_contents.values();
}

void CallChannelHandler::shutdown()
{
    //This will cause invalidated() to be emited on the 2 proxies (Tp::Channel & TpChannel)
    //we catch the tp-qt one in onCallChannelInvalidated() and the tp-glib one
    //in onTfChannelClosed() through TfChannel, which is closed when the TpChannel
    //is invalidated. When both are invalidated, we emit the channelClosed() signal.
    d->m_callChannel->requestClose();
}

//END CallChannelHandler

#include "call-channel-handler.moc"
#include "moc_call-channel-handler.cpp"
