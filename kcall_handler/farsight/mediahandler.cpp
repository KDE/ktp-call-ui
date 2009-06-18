/*
    This file is based on code from the TelepathyQt4 library.

    Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
    Copyright (C) 2009 Nokia Corporation
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#include "mediahandler.h"
#include "mediadevices.h"
#include <KDebug>
#include <TelepathyQt4/Farsight/Channel>
#include <telepathy-farsight/channel.h>

namespace Farsight {

struct MediaHandler::Private
{
    static gboolean busWatch(GstBus *bus, GstMessage *message, MediaHandler *self);
    static void onTfChannelClosed(TfChannel *tfChannel, MediaHandler *self);
    static void onSessionCreated(TfChannel *tfChannel, FsConference *conference,
                             FsParticipant *participant, MediaHandler *self);
    static void onStreamCreated(TfChannel *tfChannel, TfStream *stream, MediaHandler *self);
    static void onSrcPadAdded(TfStream *stream, GstPad *src, FsCodec *codec, MediaHandler *self);
    static gboolean onRequestResource(TfStream *stream, guint direction, MediaHandler *self);

    AbstractMediaHandler::Capabilities caps;
    Tp::StreamedMediaChannelPtr channel;
    TfChannel *tfChannel;
    GstElement *pipeline;
    AudioDevice *audioInputDevice;
    AudioDevice *audioOutputDevice;
    GstBus *bus;
    guint busWatchSrc;
};

MediaHandler::MediaHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent)
    : AbstractMediaHandler(parent), d(new Private)
{
    d->caps = None;
    d->channel = channel;
    d->tfChannel = NULL;
    d->pipeline = NULL;
    d->audioInputDevice = NULL;
    d->audioOutputDevice = NULL;
    d->bus = NULL;
    d->busWatchSrc = 0;

    initialize();
}

MediaHandler::~MediaHandler()
{
    stop();
    delete d;
}

AbstractMediaHandler::Capabilities MediaHandler::capabilities() const
{
    return d->caps;
}

AbstractAudioDevice *MediaHandler::audioInputDevice() const
{
    return d->audioInputDevice;
}

AbstractAudioDevice *MediaHandler::audioOutputDevice() const
{
    return d->audioOutputDevice;
}

void MediaHandler::initialize()
{
    static bool gInitDone = false;
    if ( !gInitDone ) {
        if ( !gst_init_check(NULL, NULL, NULL) ) {
            kError() << "Failed to initialize gstreamer";
            return;
        }
        gInitDone = true;
    }

    d->tfChannel = Tp::createFarsightChannel(d->channel);
    if (!d->tfChannel) {
        kError() << "Unable to construct TfChannel";
        return;
    }

    d->pipeline = gst_pipeline_new(NULL);
    Q_ASSERT(d->pipeline);
    d->bus = gst_pipeline_get_bus(GST_PIPELINE(d->pipeline));
    Q_ASSERT(d->bus);

    if ( (d->audioInputDevice = AudioDevice::createInputDevice(this)) ) {
        d->caps |= SendAudio;
    }

    if ( (d->audioOutputDevice = AudioDevice::createOutputDevice(this)) ) {
        d->caps |= ReceiveAudio;
    }

    /* Set up the telepathy farsight channel */
    g_signal_connect(d->tfChannel, "closed",
                     G_CALLBACK(&MediaHandler::Private::onTfChannelClosed), this);
    g_signal_connect(d->tfChannel, "session-created",
                     G_CALLBACK(&MediaHandler::Private::onSessionCreated), this);
    g_signal_connect(d->tfChannel, "stream-created",
                     G_CALLBACK(&MediaHandler::Private::onStreamCreated), this);

    gst_element_set_state(d->pipeline, GST_STATE_PLAYING);
    setStatus(Connecting);
}

void MediaHandler::stop()
{
    if (status() == Disconnected) {
        return;
    }

    if (d->tfChannel) {
        g_object_unref(d->tfChannel);
        d->tfChannel = NULL;
    }

    if (d->bus) {
        if (d->busWatchSrc != 0) {
            g_source_remove(d->busWatchSrc);
        }
        g_object_unref(d->bus);
        d->bus = 0;
    }

    delete d->audioInputDevice;
    delete d->audioOutputDevice;

    if (d->pipeline) {
        gst_element_set_state(d->pipeline, GST_STATE_NULL);
        g_object_unref(d->pipeline);
        d->pipeline = 0;
    }

    d->caps = None;
    setStatus(Disconnected);
}

gboolean MediaHandler::Private::busWatch(GstBus *bus, GstMessage *message, MediaHandler *self)
{
    Q_UNUSED(bus);
    tf_channel_bus_message(self->d->tfChannel, message);
    return TRUE;
}

void MediaHandler::Private::onTfChannelClosed(TfChannel *tfChannel, MediaHandler *self)
{
    Q_UNUSED(tfChannel);
    self->stop();
}

void MediaHandler::Private::onSessionCreated(TfChannel *tfChannel, FsConference *conference,
                                             FsParticipant *participant, MediaHandler *self)
{
    Q_UNUSED(tfChannel);
    Q_UNUSED(participant);
    self->d->busWatchSrc = gst_bus_add_watch(self->d->bus,
                                             (GstBusFunc) &MediaHandler::Private::busWatch, self);

    gst_bin_add(GST_BIN(self->d->pipeline), GST_ELEMENT(conference));
    gst_element_set_state(GST_ELEMENT(conference), GST_STATE_PLAYING);
}


void MediaHandler::Private::onStreamCreated(TfChannel *tfChannel, TfStream *stream,
                                            MediaHandler *self)
{
    Q_UNUSED(tfChannel);
    guint media_type;
    GstPad *sink;
    GstElement *element = NULL;

    g_object_get(stream, "media-type", &media_type, "sink-pad", &sink, NULL);

    switch (media_type) {
    case Tp::MediaStreamTypeAudio:
        element = self->d->audioInputDevice->bin();
        break;
    case Tp::MediaStreamTypeVideo:
        kWarning() << "Handling video is not supported yet.";
        return;
    default:
        Q_ASSERT(false);
    }

    g_signal_connect(stream, "src-pad-added",
                     G_CALLBACK(&MediaHandler::Private::onSrcPadAdded), self);
    g_signal_connect(stream, "request-resource",
                     G_CALLBACK(&MediaHandler::Private::onRequestResource), self);

    GstPad *pad = gst_element_get_static_pad(element, "src");
    gst_bin_add(GST_BIN(self->d->pipeline), element);
    gst_pad_link(pad, sink);
    gst_object_unref(sink);
    gst_element_set_state(element, GST_STATE_PLAYING);
}

void MediaHandler::Private::onSrcPadAdded(TfStream *stream, GstPad *src,
                                          FsCodec *codec, MediaHandler *self)
{
    Q_UNUSED(codec);
    guint media_type;
    GstElement *element = NULL;

    g_object_get(stream, "media-type", &media_type, NULL);

    switch (media_type) {
    case Tp::MediaStreamTypeAudio:
        element = self->d->audioOutputDevice->bin();
        break;
    case Tp::MediaStreamTypeVideo:
        kWarning() << "Handling video is not supported yet.";
        return;
    default:
        Q_ASSERT(false);
    }

    GstPad *pad = gst_element_get_static_pad(element, "sink");
    gst_bin_add(GST_BIN(self->d->pipeline), element);
    gst_pad_link(src, pad);
    gst_element_set_state(element, GST_STATE_PLAYING);

    self->setStatus(Connected);
}

gboolean MediaHandler::Private::onRequestResource(TfStream *stream, guint direction,
                                                  MediaHandler *self)
{
    guint media_type;
    g_object_get(stream, "media-type", &media_type, NULL);

    switch (media_type) {
    case Tp::MediaStreamTypeAudio:
        if ( (direction & Tp::MediaStreamDirectionSend)
              && !(self->d->caps & SendAudio) ) {
            return FALSE;
        }
        if ( (direction & Tp::MediaStreamDirectionReceive)
              && !(self->d->caps & ReceiveAudio) ) {
            return FALSE;
        }
        return TRUE;
    case Tp::MediaStreamTypeVideo:
        return FALSE;
    default:
        Q_ASSERT(false);
    }
    return FALSE; //warnings--
}

} //namespace Farsight

#include "mediahandler.moc"
