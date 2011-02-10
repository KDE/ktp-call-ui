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
#include "dtmfhandler.h"
#include "dtmfwidget.h"

struct DtmfHandler::Private
{
    Tpy::CallChannelPtr channel;
    Tp::Client::ChannelInterfaceDTMFInterface *dtmfInterface;
};

DtmfHandler::DtmfHandler(const Tpy::CallChannelPtr & channel, QObject *parent)
    : QObject(parent), d(new Private)
{
    d->channel = channel;
    d->dtmfInterface = channel->optionalInterface<Tp::Client::ChannelInterfaceDTMFInterface>();
}

DtmfHandler::~DtmfHandler()
{
    delete d;
}

void DtmfHandler::connectDtmfWidget(DtmfWidget *dtmfWidget)
{
    connect(dtmfWidget, SIGNAL(startSendDtmfEvent(Tp::DTMFEvent)),
            SLOT(onStartSendDtmfEvent(Tp::DTMFEvent)));
    connect(dtmfWidget, SIGNAL(stopSendDtmfEvent()),
            SLOT(onStopSendDtmfEvent()));
}

void DtmfHandler::onStartSendDtmfEvent(Tp::DTMFEvent event)
{
//     foreach(const Tp::StreamedMediaStreamPtr & stream, d->channel->streams()) {
//         if ( stream->type() == Tp::MediaStreamTypeAudio ) {
//             //TODO handle return value and signal on error
//             stream->startDTMFTone(event);
//             d->dtmfInterface->StartTone(stream->id(), event);
//         }
//     }
}

void DtmfHandler::onStopSendDtmfEvent()
{
//     foreach(const Tp::StreamedMediaStreamPtr & stream, d->channel->streams()) {
//         if ( stream->type() == Tp::MediaStreamTypeAudio ) {
//             //TODO handle return value and signal on error
//             stream->stopDTMFTone();
//             d->dtmfInterface->StopTone(stream->id());
//         }
//     }
}

#include "dtmfhandler.moc"
