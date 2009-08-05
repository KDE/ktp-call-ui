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
#include "dtmfhandler.h"
#include "../libkcallprivate/groupmembersmodel.h"
#include <KDebug>
#include <KLocalizedString>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingReady>

struct ChannelHandler::Private
{
    Tp::StreamedMediaChannelPtr channel;
    State state;
    AbstractMediaHandler *mediaHandler;
};

ChannelHandler::ChannelHandler(Tp::StreamedMediaChannelPtr channel, QObject *parent)
    : QObject(parent), d(new Private)
{
    d->state = NotReady;
    d->channel = channel;
    d->mediaHandler = NULL;

    Tp::PendingReady *pr = d->channel->becomeReady(QSet<Tp::Feature>()
                                                    << Tp::StreamedMediaChannel::FeatureCore
                                                    << Tp::StreamedMediaChannel::FeatureStreams);

    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChannelReady(Tp::PendingOperation*)));
    connect(d->channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*, QString, QString)));
}

ChannelHandler::~ChannelHandler()
{
    delete d;
}

void ChannelHandler::setState(State s)
{
    d->state = s;
    emit stateChanged(s);
}

void ChannelHandler::onChannelReady(Tp::PendingOperation *op)
{
    Q_ASSERT(d->state == NotReady);

    if ( op->isError() ) {
        kError() << "StreamedMediaChannel failed to become ready:"
                 << op->errorName() << op->errorMessage();
        emit logMessage(CallLog::Error, i18n("Could not initialize the channel"));
        setState(Error);
        return;
    }

    if ( d->channel->handlerStreamingRequired() ) {
        d->mediaHandler = AbstractMediaHandler::create(d->channel, this);
        connect(d->mediaHandler, SIGNAL(logMessage(CallLog::LogType, QString)),
                this, SIGNAL(logMessage(CallLog::LogType, QString)));
        emit mediaHandlerCreated(d->mediaHandler);
    }

    if ( d->channel->interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_DTMF) ) {
        kDebug() << "Creating DTMF handler";
        emit dtmfHandlerCreated(new DtmfHandler(d->channel, this));
    }

    GroupMembersModel *model = new GroupMembersModel(Tp::ChannelPtr::staticCast(d->channel), this);
    emit groupMembersModelCreated(model);

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

    Tp::MediaStreams streams = d->channel->streams();
    kDebug() << streams.size();

    foreach (const Tp::MediaStreamPtr &stream, streams) {
        onStreamAdded(stream);
    }

    //The user must have accepted the call already, using the approver.
    //No need to ask the user again...
    if ( d->channel->awaitingLocalAnswer() ) {
        d->channel->acceptCall();
    }

    setState(Connecting);
}

void ChannelHandler::onChannelInvalidated(Tp::DBusProxy *proxy, const QString &errorName,
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

void ChannelHandler::onStreamAdded(const Tp::MediaStreamPtr & stream)
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
        break;
    default:
        Q_ASSERT(false);
    }
}

void ChannelHandler::onStreamRemoved(const Tp::MediaStreamPtr & stream)
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
        break;
    default:
        Q_ASSERT(false);
    }
}

void ChannelHandler::onStreamDirectionChanged(const Tp::MediaStreamPtr & stream,
                                              Tp::MediaStreamDirection direction,
                                              Tp::MediaStreamPendingSend pendingSend)
{
    kDebug() << (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
                "stream direction changed to" << direction;
    kDebug() << "pending send:" << pendingSend;

    emit logMessage(CallLog::Information, i18nc("1 is the stream's id",
                    "Stream %1 direction changed to: %2.", stream->id(), stream->direction()));
}

void ChannelHandler::onStreamStateChanged(const Tp::MediaStreamPtr & stream,
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

void ChannelHandler::onGroupMembersChanged(const Tp::Contacts & groupMembersAdded,
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

void ChannelHandler::hangupCall()
{
    Q_ASSERT(!d->channel.isNull() && d->state != HangingUp && d->state != Disconnected && d->state != Error);
    setState(HangingUp);
    d->channel->requestClose();
}

bool ChannelHandler::requestClose()
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
    case NotReady:
        return true;
    default:
        Q_ASSERT(false);
    }
    return true; //warnings--
}

#include "channelhandler.moc"
