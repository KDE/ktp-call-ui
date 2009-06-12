/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#include "callwindowpart_p.h"
#include "pendingoutgoingcall.h"
#include "abstractmediahandler.h"
#include <KDebug>
#include <KLocalizedString>
#include <KAction>
#include <KActionCollection>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingReady>

CallWindowPartPrivate::CallWindowPartPrivate(CallWindowPart *qq)
    : QObject(qq), q_ptr(qq)
{
    m_state = Disconnected;
}

void CallWindowPartPrivate::setupActions()
{
    Q_Q(CallWindowPart);
    m_hangupAction = new KAction(KIcon("application-exit"), i18nc("@action", "Hangup"), this);
    connect(m_hangupAction, SIGNAL(triggered()), SLOT(hangupCall()));
    q->actionCollection()->addAction("hangup", m_hangupAction);
}

void CallWindowPartPrivate::setState(State s, const QString & extraMsg)
{
    Q_Q(CallWindowPart);
    switch (s) {
    case Connecting:
        setStatus(i18nc("@info:status", "Connecting..."), extraMsg);
        m_hangupAction->setEnabled(false);
        break;
    case Ringing:
        setStatus(i18nc("@info:status", "Ringing..."), extraMsg);
        m_hangupAction->setEnabled(true);
        break;
    case InCall:
        setStatus(i18nc("@info:status", "Talking..."), extraMsg);
        m_hangupAction->setEnabled(true);
        break;
    case HangingUp:
        setStatus(i18nc("@info:status", "Hanging up..."), extraMsg);
        m_hangupAction->setEnabled(false);
        break;
    case Disconnected:
        setStatus(i18nc("@info:status", "Disconnected."), extraMsg);
        m_hangupAction->setEnabled(false);
        emit q->callEnded(false);
        break;
    case Error:
        setStatus(i18nc("@info:status", "Disconnected with error."), extraMsg);
        m_hangupAction->setEnabled(false);
        emit q->callEnded(true);
        break;
    }
    m_state = s;
}

void CallWindowPartPrivate::setStatus(const QString & msg, const QString & extraMsg)
{
    Q_Q(CallWindowPart);
    emit q->setStatusBarText(QString("%1 %2").arg(msg).arg(extraMsg));
}

void CallWindowPartPrivate::callContact(Tp::ContactPtr contact)
{
    setState(Connecting);
    connect(new PendingOutgoingCall(contact, this),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(pendingOutgoingCallFinished(Tp::PendingOperation*)));
    m_contact = contact;
}

void CallWindowPartPrivate::pendingOutgoingCallFinished(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        setState(Error, op->errorMessage());
        return;
    }

    PendingOutgoingCall *pc = qobject_cast<PendingOutgoingCall*>(op);
    Q_ASSERT(pc);
    handleChannel(pc->channel());
}

void CallWindowPartPrivate::handleChannel(Tp::StreamedMediaChannelPtr channel)
{
    m_channel = channel;
    Tp::PendingReady *pr = m_channel->becomeReady(QSet<Tp::Feature>()
                                                    << Tp::StreamedMediaChannel::FeatureCore
                                                    << Tp::StreamedMediaChannel::FeatureStreams);

    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChannelReady(Tp::PendingOperation*)));
    connect(m_channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*, QString, QString)));
}

void CallWindowPartPrivate::onChannelReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "StreamedMediaChannel failed to become ready:"
                 << op->errorName() << op->errorMessage();
        setState(Error, op->errorMessage());
        return;
    }

    if ( m_channel->handlerStreamingRequired() ) {
        kDebug() << "Creating farsight channel";
        AbstractMediaHandler::create(m_channel, this);
    }

    connect(m_channel.data(),
            SIGNAL(streamAdded(Tp::MediaStreamPtr)),
            SLOT(onStreamAdded(Tp::MediaStreamPtr)));
    connect(m_channel.data(),
            SIGNAL(streamRemoved(Tp::MediaStreamPtr)),
            SLOT(onStreamRemoved(Tp::MediaStreamPtr)));
    connect(m_channel.data(),
            SIGNAL(streamDirectionChanged(Tp::MediaStreamPtr, Tp::MediaStreamDirection,
                                          Tp::MediaStreamPendingSend)),
            SLOT(onStreamDirectionChanged(Tp::MediaStreamPtr, Tp::MediaStreamDirection,
                                          Tp::MediaStreamPendingSend)));
    connect(m_channel.data(),
            SIGNAL(streamStateChanged(Tp::MediaStreamPtr, Tp::MediaStreamState)),
            SLOT(onStreamStateChanged(Tp::MediaStreamPtr, Tp::MediaStreamState)));

    Tp::MediaStreams streams = m_channel->streams();
    kDebug() << streams.size();

    foreach (const Tp::MediaStreamPtr &stream, streams) {
        kDebug() << "  type:" <<
            (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video");
        kDebug() << "  direction:" << stream->direction();
        kDebug() << "  state:" << stream->state();

       // onStreamDirectionChanged(stream, stream->direction(), stream->pendingSend());
       // onStreamStateChanged(stream, stream->state());
    }

    if ( streams.size() == 0 && !m_contact.isNull() ) {
        //HACK remove this from here
        m_channel->requestStream(m_contact, Tp::MediaStreamTypeAudio);
    }

    if ( m_channel->awaitingRemoteAnswer() ) {
        setState(Ringing);
    } else {
        setState(InCall);
    }
}

void CallWindowPartPrivate::onChannelInvalidated(Tp::DBusProxy *proxy, const QString &errorName,
                                                 const QString &errorMessage)
{
    Q_UNUSED(proxy);
    kDebug() << "channel became invalid:" << errorName << errorMessage;
    setState(Error, errorMessage);
}

void CallWindowPartPrivate::onStreamAdded(const Tp::MediaStreamPtr & stream)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") << "stream created";
    kDebug() << " direction:" << stream->direction();
    kDebug() << " state:" << stream->state();
    kDebug() << " pending send:" << stream->pendingSend();
}

void CallWindowPartPrivate::onStreamRemoved(const Tp::MediaStreamPtr & stream)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") << "stream removed";
}

void CallWindowPartPrivate::onStreamDirectionChanged(const Tp::MediaStreamPtr & stream,
                                                     Tp::MediaStreamDirection direction,
                                                     Tp::MediaStreamPendingSend pendingSend)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
                "stream direction changed to" << direction;
    kDebug() << "pending send:" << pendingSend;
}

void CallWindowPartPrivate::onStreamStateChanged(const Tp::MediaStreamPtr & stream,
                                                 Tp::MediaStreamState state)
{
    kDebug() <<  (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
                "stream state changed to" << state;
    kDebug() << " pending send:" << stream->pendingSend();
}


void CallWindowPartPrivate::hangupCall()
{
    Q_ASSERT(!m_channel.isNull() && m_state != HangingUp && m_state != Disconnected && m_state != Error);
    setState(HangingUp);
    connect(m_channel->requestClose(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChannelClosed(Tp::PendingOperation*)));
}

void CallWindowPartPrivate::onChannelClosed(Tp::PendingOperation *op)
{
    if (op->isError()) {
        kError() << "Failed to close channel:" << op->errorName() << op->errorMessage();
    } else {
        kDebug() << "Channel closed";
    }
    setState(Disconnected);
}

bool CallWindowPartPrivate::requestClose()
{
    switch(m_state) {
    case Ringing:
    case InCall:
        kDebug() << "Hanging up call on close request";
        hangupCall();
        //fall through
    case HangingUp:
        return false;
    case Connecting: //FIXME is it ok to just close the window here?
    case Disconnected:
    case Error:
        return true;
    default:
        Q_ASSERT(false);
    }
    return true; //warnings--
}

#include "callwindowpart_p.moc"
