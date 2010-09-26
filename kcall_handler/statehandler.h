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
#ifndef STATEHANDLER_H
#define STATEHANDLER_H

#include "calllog.h"
#include <TelepathyQt4/StreamedMediaChannel>

class StateHandler : public QObject
{
    Q_OBJECT
public:
    explicit StateHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent = 0);
    virtual ~StateHandler();

    enum State { Connecting, Ringing, InCall, HangingUp, Disconnected, Error };
    bool requestClose();
    void setSendVideo(bool enabled);

public slots:
    void hangupCall();

signals:
    void stateChanged(StateHandler::State newState);
    void logMessage(CallLog::LogType logType, const QString & message);
    void audioStreamAdded();
    void audioStreamRemoved();
    void videoStreamAdded();
    void videoStreamRemoved();
    void sendVideoStateChanged(bool enabled);

private:
    void setState(State s);

private slots:
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
    void onPendingMediaStreamFinished(Tp::PendingOperation *op);

private:
    struct Private;
    Private *const d;
};

#endif
