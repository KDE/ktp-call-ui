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
#include "../../libkgstdevices/devicefactory.h"
#include "../../libkgstdevices/mediadevices.h"
#include <KDebug>
#include <KGlobal>
#include <KSharedConfig>
#include <KConfigGroup>
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

    GstPad *newAudioSinkPad();

    Tp::StreamedMediaChannelPtr channel;
    TfChannel *tfChannel;
    GstElement *pipeline;
    GstElement *liveadder;
    KGstDevices::DeviceFactory *deviceFactory;
    GstBus *bus;
    guint busWatchSrc;
};

MediaHandler::MediaHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent)
    : AbstractMediaHandler(parent), d(new Private)
{
    d->channel = channel;
    d->tfChannel = NULL;
    d->pipeline = NULL;
    d->liveadder = NULL;
    d->deviceFactory = NULL;
    d->bus = NULL;
    d->busWatchSrc = 0;

    qRegisterMetaType<VolumeControlInterface*>("VolumeControlInterface*");
    initialize();
}

MediaHandler::~MediaHandler()
{
    stop();

    if (d->tfChannel) {
        g_object_unref(d->tfChannel);
        d->tfChannel = NULL;
    }

    if (d->bus) {
        g_object_unref(d->bus);
        d->bus = NULL;
    }

    delete d->deviceFactory;

    if (d->pipeline) {
        g_object_unref(d->pipeline);
        d->pipeline = NULL;
        d->liveadder = NULL;
    }

    delete d;
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

    /* Set up the telepathy farsight channel */
    g_signal_connect(d->tfChannel, "closed",
                     G_CALLBACK(&MediaHandler::Private::onTfChannelClosed), this);
    g_signal_connect(d->tfChannel, "session-created",
                     G_CALLBACK(&MediaHandler::Private::onSessionCreated), this);
    g_signal_connect(d->tfChannel, "stream-created",
                     G_CALLBACK(&MediaHandler::Private::onStreamCreated), this);

    d->pipeline = gst_pipeline_new(NULL);
    Q_ASSERT(d->pipeline);
    d->bus = gst_pipeline_get_bus(GST_PIPELINE(d->pipeline));
    Q_ASSERT(d->bus);
    gst_element_set_state(d->pipeline, GST_STATE_PLAYING);

    const KConfigGroup deviceSettings = KGlobal::config()->group("Gstreamer Devices");
    d->deviceFactory = new KGstDevices::DeviceFactory(deviceSettings);
}

void MediaHandler::stop()
{
    if (d->bus) {
        if (d->busWatchSrc != 0) {
            g_source_remove(d->busWatchSrc);
        }
    }

    if (d->pipeline) {
        gst_element_set_state(d->pipeline, GST_STATE_NULL);
    }

    QMetaObject::invokeMethod(this, "audioInputDeviceDestroyed", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "audioOutputDeviceDestroyed", Qt::QueuedConnection);
}

void MediaHandler::onAudioSrcPadAdded(GstPad *srcPad)
{
    if ( !d->deviceFactory->audioOutputDevice() ) {
        if ( !d->deviceFactory->createAudioOutputDevice() ) {
            foreach(const QString & error, d->deviceFactory->errorLog()) {
                QMetaObject::invokeMethod(this, "logMessage", Qt::QueuedConnection,
                                          Q_ARG(CallLog::LogType, CallLog::Error),
                                          Q_ARG(QString, error));
            }
            d->deviceFactory->clearLog();
            return;
        }
        gst_bin_add(GST_BIN(d->pipeline), d->deviceFactory->audioOutputDevice()->bin());
        gst_element_set_state(d->deviceFactory->audioOutputDevice()->bin(), GST_STATE_PLAYING);
    }

    GstPad *sinkPad = d->newAudioSinkPad();
    Q_ASSERT(sinkPad);
    gst_pad_link(srcPad, sinkPad);

    QMetaObject::invokeMethod(this, "audioOutputDeviceCreated", Qt::QueuedConnection,
                              Q_ARG(VolumeControlInterface*, d->deviceFactory->audioOutputDevice()));
}

void MediaHandler::onAudioSinkPadAdded(GstPad *sinkPad)
{
    if ( !d->deviceFactory->audioInputDevice() ) {
        if ( !d->deviceFactory->createAudioInputDevice() ) {
            foreach(const QString & error, d->deviceFactory->errorLog()) {
                QMetaObject::invokeMethod(this, "logMessage", Qt::QueuedConnection,
                                          Q_ARG(CallLog::LogType, CallLog::Error),
                                          Q_ARG(QString, error));
            }
            d->deviceFactory->clearLog();
            return;
        }
        gst_bin_add(GST_BIN(d->pipeline), d->deviceFactory->audioInputDevice()->bin());
        gst_element_set_state(d->deviceFactory->audioInputDevice()->bin(), GST_STATE_PLAYING);
    }

    GstPad *srcPad = gst_element_get_static_pad(d->deviceFactory->audioInputDevice()->bin(), "src");
    Q_ASSERT(srcPad);
    gst_pad_link(srcPad, sinkPad);
    gst_object_unref(srcPad);

    QMetaObject::invokeMethod(this, "audioInputDeviceCreated", Qt::QueuedConnection,
                              Q_ARG(VolumeControlInterface*, d->deviceFactory->audioInputDevice()));
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
    g_signal_connect(stream, "src-pad-added",
                     G_CALLBACK(&MediaHandler::Private::onSrcPadAdded), self);
    g_signal_connect(stream, "request-resource",
                     G_CALLBACK(&MediaHandler::Private::onRequestResource), self);
}

void MediaHandler::Private::onSrcPadAdded(TfStream *stream, GstPad *src,
                                          FsCodec *codec, MediaHandler *self)
{
    Q_UNUSED(codec);
    guint media_type;
    g_object_get(stream, "media-type", &media_type, NULL);

    switch (media_type) {
    case Tp::MediaStreamTypeAudio:
        self->onAudioSrcPadAdded(src);
        break;
    case Tp::MediaStreamTypeVideo:
        kWarning() << "Handling video is not supported yet.";
        return;
    default:
        Q_ASSERT(false);
    }
}

gboolean MediaHandler::Private::onRequestResource(TfStream *stream, guint direction,
                                                  MediaHandler *self)
{
    guint media_type;
    g_object_get(stream, "media-type", &media_type, NULL);

    switch (media_type) {
    case Tp::MediaStreamTypeAudio:

        switch(direction) {
        case Tp::MediaStreamDirectionSend:
            GstPad *sink;
            g_object_get(stream, "sink-pad", &sink, NULL);
            self->onAudioSinkPadAdded(sink);
            gst_object_unref(sink);
            return (self->d->deviceFactory->audioInputDevice() != NULL);

        case Tp::MediaStreamDirectionReceive:
            /* The tp-farsight api is broken, so this signal will come after
               the src-pad-added signal, which will allocate the resource instead.
               Here we just return whether src-pad-added could allocate the device or not. */
            return (self->d->deviceFactory->audioOutputDevice() != NULL);

        default:
            Q_ASSERT(false);
        }
        break;

    case Tp::MediaStreamTypeVideo:
        return FALSE;

    default:
        Q_ASSERT(false);
    }
    return FALSE; //warnings--
}

GstPad *MediaHandler::Private::newAudioSinkPad()
{
    if ( !liveadder ) {
        if ( !(liveadder = gst_element_factory_make("liveadder", NULL)) ) {
            kDebug() << "liveadder not available. Attempting to use plain adder";
            if ( !(liveadder = gst_element_factory_make("adder", NULL)) ) {
                kFatal() << "Failed to create an adder gstreamer element. Please install "
                            "either the \"adder\" or the \"liveadder\" gstreamer plugin.";
            }
        }

        gst_bin_add(GST_BIN(pipeline), liveadder);
        gst_bin_add(GST_BIN(pipeline), deviceFactory->audioOutputDevice()->bin());
        gst_element_link(liveadder, deviceFactory->audioOutputDevice()->bin());

        gst_element_set_state(liveadder, GST_STATE_PLAYING);
        gst_element_set_state(deviceFactory->audioOutputDevice()->bin(), GST_STATE_PLAYING);
    }
    return gst_element_get_request_pad(liveadder, "sink%d");
}

} //namespace Farsight

#include "mediahandler.moc"
