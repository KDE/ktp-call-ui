/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
    Copyright (C) 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "call-content-handler.h"

#include "private/tf-audio-content-handler.h"
#include "private/tf-video-content-handler.h"
#include "private/sink-controllers.h"

#include <KDebug>

using namespace KTpCallPrivate;

//BEGIN CallContentHandler

struct CallContentHandler::Private
{
    TfContentHandler *contentHandler;
};

CallContentHandler::CallContentHandler(TfContentHandler *handler, QObject *parent)
    : QObject(parent), d(new Private)
{
    d->contentHandler = handler;
    connect(d->contentHandler, SIGNAL(localSendingStateChanged(bool)),
            this, SIGNAL(localSendingStateChanged(bool)));
    connect(d->contentHandler, SIGNAL(remoteSendingStateChanged(Tp::ContactPtr,bool)),
            this, SIGNAL(remoteSendingStateChanged(Tp::ContactPtr,bool)));
}

CallContentHandler::~CallContentHandler()
{
    delete d;
}

Tp::CallContentPtr CallContentHandler::callContent() const
{
    return d->contentHandler->callContent();
}

Tp::Contacts CallContentHandler::remoteMembers() const
{
    return d->contentHandler->remoteMembers();
}

//END CallContentHandler
//BEGIN AudioContentHandler

AudioContentHandler::AudioContentHandler(TfAudioContentHandler *handler, QObject *parent)
    : CallContentHandler(handler, parent)
{
}

VolumeController *AudioContentHandler::inputVolumeControl() const
{
    return static_cast<TfAudioContentHandler*>(d->contentHandler)->inputVolumeController();
}

VolumeController *AudioContentHandler::outputVolumeControl() const
{
    return static_cast<TfAudioContentHandler*>(d->contentHandler)->outputVolumeController();
}

//END AudioContentHandler
//BEGIN VideoContentHandler

VideoContentHandler::VideoContentHandler(TfVideoContentHandler *handler, QObject *parent)
    : CallContentHandler(handler, parent)
{
}

void VideoContentHandler::linkVideoPreviewSink(const QGst::ElementPtr & sink)
{
    static_cast<TfVideoContentHandler*>(d->contentHandler)->linkVideoPreviewSink(sink);
}

void VideoContentHandler::unlinkVideoPreviewSink()
{
    static_cast<TfVideoContentHandler*>(d->contentHandler)->unlinkVideoPreviewSink();
}

void VideoContentHandler::linkRemoteMemberVideoSink(const Tp::ContactPtr & contact,
                                                    const QGst::ElementPtr & sink)
{
    BaseSinkController *ctrl = d->contentHandler->sinkController(contact);
    if (ctrl) {
        static_cast<VideoSinkController*>(ctrl)->linkVideoSink(sink);
    }
}

void VideoContentHandler::unlinkRemoteMemberVideoSink(const Tp::ContactPtr & contact)
{
    BaseSinkController *ctrl = d->contentHandler->sinkController(contact);
    if (ctrl) {
        static_cast<VideoSinkController*>(ctrl)->unlinkVideoSink();
    }
}

//END VideoContentHandler
