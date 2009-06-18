/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "channelhandler.h"
#include "abstractmediahandler.h"
#include <KDebug>
#include <KLocalizedString>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingReady>

ChannelHandler::ChannelHandler(QObject *parent)
    : QObject(parent)
{
    m_state = Disconnected;
}

void ChannelHandler::setState(State s)
{
    m_state = s;
    emit stateChanged(s);
    switch (s) {
    case Disconnected:
        emit callEnded(false);
        break;
    case Error:
        emit callEnded(true);
        break;
    default:
        break;
    }
}

void ChannelHandler::handleChannel(Tp::StreamedMediaChannelPtr channel)
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

void ChannelHandler::onChannelReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "StreamedMediaChannel failed to become ready:"
                 << op->errorName() << op->errorMessage();
        setState(Error);
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

    if ( m_channel->awaitingRemoteAnswer() ) {
        setState(Ringing);
    } else {
        setState(InCall);
    }
}

void ChannelHandler::onChannelInvalidated(Tp::DBusProxy *proxy, const QString &errorName,
                                                 const QString &errorMessage)
{
    Q_UNUSED(proxy);
    kDebug() << "channel became invalid:" << errorName << errorMessage;
    setState(Error);
}

void ChannelHandler::onStreamAdded(const Tp::MediaStreamPtr & stream)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") << "stream created";
    kDebug() << " direction:" << stream->direction();
    kDebug() << " state:" << stream->state();
    kDebug() << " pending send:" << stream->pendingSend();
}

void ChannelHandler::onStreamRemoved(const Tp::MediaStreamPtr & stream)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") << "stream removed";
}

void ChannelHandler::onStreamDirectionChanged(const Tp::MediaStreamPtr & stream,
                                                     Tp::MediaStreamDirection direction,
                                                     Tp::MediaStreamPendingSend pendingSend)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
                "stream direction changed to" << direction;
    kDebug() << "pending send:" << pendingSend;
}

void ChannelHandler::onStreamStateChanged(const Tp::MediaStreamPtr & stream,
                                                 Tp::MediaStreamState state)
{
    kDebug() <<  (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
                "stream state changed to" << state;
    kDebug() << " pending send:" << stream->pendingSend();
}


void ChannelHandler::hangupCall()
{
    Q_ASSERT(!m_channel.isNull() && m_state != HangingUp && m_state != Disconnected && m_state != Error);
    setState(HangingUp);
    connect(m_channel->requestClose(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChannelClosed(Tp::PendingOperation*)));
}

void ChannelHandler::onChannelClosed(Tp::PendingOperation *op)
{
    if (op->isError()) {
        kError() << "Failed to close channel:" << op->errorName() << op->errorMessage();
    } else {
        kDebug() << "Channel closed";
    }
    setState(Disconnected);
}

bool ChannelHandler::requestClose()
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

#include "channelhandler.moc"
