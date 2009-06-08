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
#ifndef CALLWINDOWPART_P_H
#define CALLWINDOWPART_P_H

#include "callwindowpart.h"
class QStackedWidget;
class QLabel;
class KAction;

class CallWindowPartPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(CallWindowPart)
public:
    CallWindowPartPrivate(CallWindowPart *qq);

    enum State { Connecting, Ringing, InCall, HangingUp, Disconnected, Error };
    void setupActions();
    void callContact(Tp::ContactPtr contact);
    void handleChannel(Tp::StreamedMediaChannelPtr channel);
    bool requestClose();

    //FIXME remove those from here
    QStackedWidget *videoStackedWidget;
    QLabel *videoPlaceHolder;

private:
    void setState(State s, const QString & extraMsg = QString());
    void setStatus(const QString & msg, const QString & extraMsg);

private slots:
    void pendingOutgoingCallFinished(Tp::PendingOperation *op);
    void onChannelReady(Tp::PendingOperation *op);
    void hangupCall();
    void onChannelClosed(Tp::PendingOperation *op);

private:
    CallWindowPart *const q_ptr;

    Tp::StreamedMediaChannelPtr m_channel;
    State m_state;
    KAction *m_hangupAction;
};

#endif
