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
#ifndef DTMF_HANDLER_H
#define DTMF_HANDLER_H

#include <TelepathyQt/CallChannel>

class DtmfQml;

class DtmfHandler : public QObject
{
    Q_OBJECT
public:
    explicit DtmfHandler(const Tp::CallChannelPtr & channel, QObject *parent = 0);
    virtual ~DtmfHandler();

    void connectDtmfQml(DtmfQml *dtmfQml);

private Q_SLOTS:
    void onStartSendDtmfEvent(Tp::DTMFEvent event);
    void onStopSendDtmfEvent();

private:
    struct Private;
    Private *const d;
};

#endif
