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

void DtmfHandler::connectDtmfWidget(DtmfWidget *dtmfWidget)
{
    connect(dtmfWidget, SIGNAL(startSendDtmfEvent(Tp::DTMFEvent)),
            SLOT(onStartSendDtmfEvent(Tp::DTMFEvent)));
    connect(dtmfWidget, SIGNAL(stopSendDtmfEvent()),
            SLOT(onStopSendDtmfEvent()));
}

void DtmfHandler::onStartSendDtmfEvent(Tp::DTMFEvent event)
{
    Q_FOREACH(const Tp::CallContentPtr & content, d->channel->contentsForType(Tp::MediaStreamTypeAudio)) {
        content->startDTMFTone(event);
    }
}

void DtmfHandler::onStopSendDtmfEvent()
{
    Q_FOREACH(const Tp::CallContentPtr & content, d->channel->contentsForType(Tp::MediaStreamTypeAudio)) {
        content->stopDTMFTone();
    }
}

#include "dtmfhandler.moc"
