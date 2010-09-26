/*
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

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
#include "statehandler.h"
#include <KDebug>
#include <KLocalizedString>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingReady>

struct StateHandler::Private
{
    Tp::StreamedMediaChannelPtr channel;
    State state;
};

StateHandler::StateHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent)
    : QObject(parent), d(new Private)
{
    d->state = Connecting;
    d->channel = channel;

    connect(d->channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*, QString, QString)));
    connect(d->channel.data(),
            SIGNAL(streamAdded(Tp::MediaStreamPtr)),
            SLOT(onStreamAdded(Tp::MediaStreamPtr)));
    connect(d->channel.data(),
            SIGNAL(streamRemoved(Tp::MediaStreamPtr)),
            SLOT(onStreamRemoved(Tp::MediaStreamPtr)));
    connect(d->channel.data(),
            SIGNAL(streamDirectionChanged(Tp::MediaStreamPtr, Tp::MediaStreamDirection,
                                          Tp::MediaStreamPendingSend)),
            SLOT(onStreamDirectionChanged(Tp::MediaStreamPtr, Tp::MediaStreamDirection,
                                          Tp::MediaStreamPendingSend)));
    connect(d->channel.data(),
            SIGNAL(streamStateChanged(Tp::MediaStreamPtr, Tp::MediaStreamState)),
            SLOT(onStreamStateChanged(Tp::MediaStreamPtr, Tp::MediaStreamState)));
    connect(d->channel.data(),
            SIGNAL(groupMembersChanged(Tp::Contacts, Tp::Contacts, Tp::Contacts,
                                       Tp::Contacts, Tp::Channel::GroupMemberChangeDetails)),
            SLOT(onGroupMembersChanged(Tp::Contacts, Tp::Contacts, Tp::Contacts,
                                       Tp::Contacts, Tp::Channel::GroupMemberChangeDetails)));
}

StateHandler::~StateHandler()
{
    delete d;
}

void StateHandler::init()
{
    kDebug() << "Initializing the state handler";

    Tp::MediaStreams streams = d->channel->streams();
    kDebug() << streams.size();

    foreach (const Tp::MediaStreamPtr &stream, streams) {
        onStreamAdded(stream);
    }
}

void StateHandler::setState(State s)
{
    d->state = s;
    emit stateChanged(s);
}

void StateHandler::onChannelInvalidated(Tp::DBusProxy *proxy, const QString &errorName,
                                          const QString &errorMessage)
{
    Q_UNUSED(proxy);
    kDebug() << "channel became invalid:" << errorName << errorMessage;

    if ( errorName == TELEPATHY_ERROR_CANCELLED ) {
        emit logMessage(CallLog::Information, i18n("Channel closed"));
        setState(Disconnected);
    } else {
        emit logMessage(CallLog::Error, i18n("Channel closed with error: %1", errorMessage));
        setState(Error);
    }
}

void StateHandler::onStreamAdded(const Tp::MediaStreamPtr & stream)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") << "stream created";
    kDebug() << " direction:" << stream->direction();
    kDebug() << " state:" << stream->state();
    kDebug() << " pending send:" << stream->pendingSend();

    if ( stream->type() == Tp::MediaStreamTypeAudio ) {
        emit logMessage(CallLog::Information, i18n("Audio stream created. Stream id: %1.",
                                                    stream->id()));
    } else {
        emit logMessage(CallLog::Information, i18n("Video stream created. Stream id: %1.",
                                                    stream->id()));
    }

    emit logMessage(CallLog::Information, i18nc("1 is the stream's id",
                    "Stream %1 direction is: %2.", stream->id(), stream->direction()));

    //clear pending send in new video streams.
    if ( stream->type() == Tp::MediaStreamTypeVideo && stream->pendingSend() ) {
        stream->requestDirection(Tp::MediaStreamDirectionReceive);
    }

    switch (stream->type()) {
    case Tp::MediaStreamTypeAudio:
        emit audioStreamAdded();
        break;
    case Tp::MediaStreamTypeVideo:
        emit videoStreamAdded();
        if ( stream->direction() & Tp::MediaStreamDirectionSend ) {
            emit sendVideoStateChanged(true);
        } else {
            emit sendVideoStateChanged(false);
        }
        break;
    default:
        Q_ASSERT(false);
    }
}

void StateHandler::onStreamRemoved(const Tp::MediaStreamPtr & stream)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") << "stream removed";
    emit logMessage(CallLog::Information, i18nc("1 is the stream's id",
                    "Stream %1 removed.", stream->id()));

    switch (stream->type()) {
    case Tp::MediaStreamTypeAudio:
        emit audioStreamRemoved();
        break;
    case Tp::MediaStreamTypeVideo:
        emit videoStreamRemoved();
        emit sendVideoStateChanged(false);
        break;
    default:
        Q_ASSERT(false);
    }
}

void StateHandler::onStreamDirectionChanged(const Tp::MediaStreamPtr & stream,
                                              Tp::MediaStreamDirection direction,
                                              Tp::MediaStreamPendingSend pendingSend)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
                "stream direction changed to" << direction;
    kDebug() << "pending send:" << pendingSend;

    emit logMessage(CallLog::Information, i18nc("1 is the stream's id",
                    "Stream %1 direction changed to: %2.", stream->id(), stream->direction()));

    if ( stream->type() == Tp::MediaStreamTypeVideo ) {
        if ( direction & Tp::MediaStreamDirectionSend ) {
            emit sendVideoStateChanged(true);
        } else {
            emit sendVideoStateChanged(false);
        }
    }
}

void StateHandler::onStreamStateChanged(const Tp::MediaStreamPtr & stream,
                                          Tp::MediaStreamState state)
{
    kDebug() <<  (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
                "stream state changed to" << state;
    kDebug() << " pending send:" << stream->pendingSend();

    emit logMessage(CallLog::Information, i18nc("1 is the stream's id",
                    "Stream %1 state changed to: %2.", stream->id(), state));

    if (d->state == Connecting && state == Tp::MediaStreamStateConnected) {
        if ( d->channel->awaitingRemoteAnswer() ) {
            setState(Ringing);
        } else {
            setState(InCall);
        }
    }
}

void StateHandler::onGroupMembersChanged(const Tp::Contacts & groupMembersAdded,
                                           const Tp::Contacts & groupLocalPendingMembersAdded,
                                           const Tp::Contacts & groupRemotePendingMembersAdded,
                                           const Tp::Contacts & groupMembersRemoved,
                                           const Tp::Channel::GroupMemberChangeDetails & details)
{
    Q_UNUSED(groupLocalPendingMembersAdded);
    Q_UNUSED(details);

    foreach(const Tp::ContactPtr & contact, groupMembersAdded) {
        emit logMessage(CallLog::Information, i18n("User %1 joined the conference.", contact->alias()));
    }
    foreach(const Tp::ContactPtr & contact, groupRemotePendingMembersAdded) {
        emit logMessage(CallLog::Information, i18n("Awaiting answer from user %1.", contact->alias()));
    }
    foreach(const Tp::ContactPtr & contact, groupMembersRemoved) {
        emit logMessage(CallLog::Information, i18n("User %1 left the conference.", contact->alias()));
    }

    if ( d->state == Ringing ) {
        if ( groupMembersAdded.size() > 0 && !d->channel->awaitingRemoteAnswer() ) {
            setState(InCall);
        }
    }
}

void StateHandler::hangupCall()
{
    Q_ASSERT(!d->channel.isNull() && d->state != HangingUp && d->state != Disconnected && d->state != Error);
    setState(HangingUp);
    d->channel->requestClose();
}

bool StateHandler::requestClose()
{
    switch(d->state) {
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

void StateHandler::setSendVideo(bool enabled)
{
    foreach (const Tp::MediaStreamPtr &stream, d->channel->streams()) {
        if ( stream->type() == Tp::MediaStreamTypeVideo ) {
            int newDirection;
            if ( enabled ) {
                newDirection = stream->direction() | Tp::MediaStreamDirectionSend;
            } else {
                newDirection = stream->direction() & ~Tp::MediaStreamDirectionSend;
            }
            stream->requestDirection(static_cast<Tp::MediaStreamDirection>(newDirection));
            return;
        }
    }

    if (enabled) {
        foreach (const Tp::ContactPtr & contact, d->channel->groupContacts()) {
            Tp::PendingMediaStreams *ps = d->channel->requestStream(contact, Tp::MediaStreamTypeVideo);
            connect(ps, SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onPendingMediaStreamFinished(Tp::PendingOperation*)));
        }
    }
}

//This is called from setSendVideo when a new sending video stream is to be created
void StateHandler::onPendingMediaStreamFinished(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "Failed to request video stream" << op->errorName() << op->errorMessage();
        return;
    }

    Tp::PendingMediaStreams *ps = qobject_cast<Tp::PendingMediaStreams*>(op);
    foreach(const Tp::MediaStreamPtr & stream, ps->streams()) {
        stream->requestDirection(Tp::MediaStreamDirectionSend);
    }
}

#include "statehandler.moc"
