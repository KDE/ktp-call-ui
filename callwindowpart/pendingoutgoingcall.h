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
#ifndef PENDINGOUTGOINGCALL_H
#define PENDINGOUTGOINGCALL_H

#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/StreamedMediaChannel>

class PendingOutgoingCall : public Tp::PendingOperation
{
    Q_OBJECT
public:
    PendingOutgoingCall(Tp::ContactPtr contact, QObject *parent = 0);
    virtual ~PendingOutgoingCall();

    Tp::StreamedMediaChannelPtr channel() const;

private slots:
    void onOutgoingConnectionReady(Tp::PendingOperation *op);
    void onCallRequestFinished(Tp::PendingOperation *op);
    void onOutgoingChannelReady(Tp::PendingOperation *op);

private:
    struct Private;
    Private *const d;
};

#endif
