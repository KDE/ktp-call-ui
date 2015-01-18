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
#include "dtmf-handler.h"
#include "dtmf-qml.h"

struct DtmfHandler::Private
{
    Tp::CallChannelPtr channel;
};

DtmfHandler::DtmfHandler(const Tp::CallChannelPtr & channel, QObject *parent)
    : QObject(parent), d(new Private)
{
    d->channel = channel;
}

DtmfHandler::~DtmfHandler()
{
    delete d;
}

//! Connects the GUI of the \a Dialpad with the internal logic of \a DtmfHandler. \a Ekaitz
void DtmfHandler::connectDtmfQml(DtmfQml *dtmfQml)
{
    connect(dtmfQml, SIGNAL(startSendDtmfEvent(Tp::DTMFEvent)), SLOT(onStartSendDtmfEvent(Tp::DTMFEvent)));
    connect(dtmfQml, SIGNAL(stopSendDtmfEvent()), SLOT(onStopSendDtmfEvent()));
}

void DtmfHandler::onStartSendDtmfEvent(Tp::DTMFEvent event)
{
    Tp::Client::CallContentInterfaceDTMFInterface *dtmfInterface = 0;
    Q_FOREACH(const Tp::CallContentPtr & content, d->channel->contentsForType(Tp::MediaStreamTypeAudio)) {
        dtmfInterface = content->interface<Tp::Client::CallContentInterfaceDTMFInterface>();
        if (dtmfInterface) {
            dtmfInterface->StartTone(event);
        }
    }
}

void DtmfHandler::onStopSendDtmfEvent()
{
    Tp::Client::CallContentInterfaceDTMFInterface *dtmfInterface = 0;
    Q_FOREACH(const Tp::CallContentPtr & content, d->channel->contentsForType(Tp::MediaStreamTypeAudio)) {
        dtmfInterface = content->interface<Tp::Client::CallContentInterfaceDTMFInterface>();
        if (dtmfInterface) {
            dtmfInterface->StopTone();
        }
    }
}
