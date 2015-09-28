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

#include "call-channel-handler.h"

#include "private/tf-channel-handler.h"
#include "private/tf-audio-content-handler.h"
#include "private/tf-video-content-handler.h"

#include "libktpcall_debug.h"

using namespace KTpCallPrivate;

class StandardTfContentHandlerFactory : public TfContentHandlerFactory
{
public:
    virtual TfContentHandler *createContentHandler(
            const QTf::ContentPtr & tfContent, TfChannelHandler *parent)
    {
        switch (tfContent->property("media-type").toInt()) {
        case Tp::MediaStreamTypeAudio:
            return new TfAudioContentHandler(tfContent, parent);
        case Tp::MediaStreamTypeVideo:
            return new TfVideoContentHandler(tfContent, parent);
        default:
            Q_ASSERT(false);
            return NULL;
        }
    }

    static TfContentHandlerFactory *construct() {
        return new StandardTfContentHandlerFactory;
    }
};


struct CallChannelHandler::Private
{
    TfChannelHandler *channelHandler;
    QHash<TfContentHandler*, CallContentHandler*> contents;
};

CallChannelHandler::CallChannelHandler(const Tp::CallChannelPtr & channel, QObject *parent)
    : QObject(parent), d(new Private)
{
    d->channelHandler = new TfChannelHandler(channel,
            &StandardTfContentHandlerFactory::construct, this);
    connect(d->channelHandler, SIGNAL(channelClosed()), this, SIGNAL(channelClosed()));
    connect(d->channelHandler, SIGNAL(contentAdded(KTpCallPrivate::TfContentHandler*)),
            SLOT(_k_onContentAdded(KTpCallPrivate::TfContentHandler*)));
    connect(d->channelHandler, SIGNAL(contentRemoved(KTpCallPrivate::TfContentHandler*)),
            SLOT(_k_onContentRemoved(KTpCallPrivate::TfContentHandler*)));
}

CallChannelHandler::~CallChannelHandler()
{
    delete d;
}

QList<CallContentHandler*> CallChannelHandler::contents() const
{
    return d->contents.values();
}

void CallChannelHandler::shutdown()
{
    d->channelHandler->shutdown();
}

void CallChannelHandler::_k_onContentAdded(TfContentHandler *tfContentHandler)
{
    connect(tfContentHandler,
            SIGNAL(callContentReady(KTpCallPrivate::TfContentHandler*)),
            SLOT(_k_onContentReady(KTpCallPrivate::TfContentHandler*)));
}

void CallChannelHandler::_k_onContentReady(TfContentHandler *tfContentHandler)
{
    CallContentHandler *callContentHandler = 0;

    TfAudioContentHandler *tfAudioContentHandler = qobject_cast<TfAudioContentHandler*>(tfContentHandler);
    if (tfAudioContentHandler) {
        callContentHandler = new AudioContentHandler(tfAudioContentHandler, this);
    }

    TfVideoContentHandler *tfVideoContentHandler = qobject_cast<TfVideoContentHandler*>(tfContentHandler);
    if (tfVideoContentHandler) {
        callContentHandler = new VideoContentHandler(tfVideoContentHandler, this);
    }

    Q_ASSERT(callContentHandler);
    d->contents.insert(tfContentHandler, callContentHandler);
    Q_EMIT contentAdded(callContentHandler);
}

void CallChannelHandler::_k_onContentRemoved(TfContentHandler *tfContentHandler)
{
    if (d->contents.contains(tfContentHandler)) {
        CallContentHandler *callContentHandler = d->contents.take(tfContentHandler);
        Q_EMIT contentRemoved(callContentHandler);
        delete callContentHandler;
    } else {
        qCWarning(LIBKTPCALL) << "Unknown content removed";
    }
}
