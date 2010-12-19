/*
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
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
#ifndef CALLCHANNELHANDLER_H
#define CALLCHANNELHANDLER_H

#include "callparticipant.h"
class CallChannelHandlerPrivate;

class CallChannelHandler : public QObject
{
    Q_OBJECT
public:
    CallChannelHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent = 0);
    virtual ~CallChannelHandler();

    QList<CallParticipant*> participants() const;

public Q_SLOTS:
    void hangup(const QString & message = QString());

Q_SIGNALS:
    void participantJoined(CallParticipant *participant);
    void participantLeft(CallParticipant *participant);
    void error(const QString & message);
    void callEnded(const QString & reason);

private:
    friend class CallChannelHandlerPrivate;
    CallChannelHandlerPrivate *const d;
};

#endif // CALLCHANNELHANDLER_H
