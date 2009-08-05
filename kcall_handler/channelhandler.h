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
#ifndef CHANNELHANDLER_H
#define CHANNELHANDLER_H

#include "calllog.h"
#include <TelepathyQt4/StreamedMediaChannel>
class AbstractMediaHandler;
class GroupMembersModel;
class DtmfHandler;

class ChannelHandler : public QObject
{
    Q_OBJECT
public:
    explicit ChannelHandler(Tp::StreamedMediaChannelPtr channel, QObject *parent = 0);
    virtual ~ChannelHandler();

    enum State { NotReady, Connecting, Ringing, InCall, HangingUp, Disconnected, Error };
    bool requestClose();

public slots:
    void hangupCall();

signals:
    void stateChanged(ChannelHandler::State newState);
    void mediaHandlerCreated(AbstractMediaHandler *handler);
    void groupMembersModelCreated(GroupMembersModel *model);
    void dtmfHandlerCreated(DtmfHandler *handler);
    void logMessage(CallLog::LogType logType, const QString & message);
    void audioStreamAdded();
    void audioStreamRemoved();
    void videoStreamAdded();
    void videoStreamRemoved();

private:
    void setState(State s);

private slots:
    void onChannelReady(Tp::PendingOperation *op);
    void onChannelInvalidated(Tp::DBusProxy *proxy, const QString &errorName,
                              const QString &errorMessage);
    void onStreamAdded(const Tp::MediaStreamPtr & stream);
    void onStreamRemoved(const Tp::MediaStreamPtr & stream);
    void onStreamDirectionChanged(const Tp::MediaStreamPtr & stream,
                                  Tp::MediaStreamDirection direction,
                                  Tp::MediaStreamPendingSend pendingSend);
    void onStreamStateChanged(const Tp::MediaStreamPtr & stream, Tp::MediaStreamState state);
    void onGroupMembersChanged(const Tp::Contacts & groupMembersAdded,
                               const Tp::Contacts & groupLocalPendingMembersAdded,
                               const Tp::Contacts & groupRemotePendingMembersAdded,
                               const Tp::Contacts & groupMembersRemoved,
                               const Tp::Channel::GroupMemberChangeDetails & details);

private:
    struct Private;
    Private *const d;
};

#endif
