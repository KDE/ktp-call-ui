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
#include "qtfchannel.h"
#include "../libqtgstreamer/qgstbus.h"
#include <QtCore/QMutex>
#include <TelepathyQt4/Farsight/Channel>
#include <telepathy-farsight/channel.h>
#include <gst/farsight/fs-codec.h>

using namespace QtGstreamer;

class QTfChannel::Private
{
public:
    Private(QTfChannel *qq) : q(qq) {}
    ~Private();

    void init(Tp::StreamedMediaChannelPtr channel, QGstBusPtr bus);

    //slots
    void onBusMessage(GstMessage *message);
    void onAudioSrcPadAdded(QtGstreamer::QGstPadPtr pad);
    void onVideoSrcPadAdded(QtGstreamer::QGstPadPtr pad);
    void onConferencePadRemoved(QtGstreamer::QGstPadPtr pad);

    static gboolean busWatch(GstBus *bus, GstMessage *message, QTfChannel::Private *self);
    static void onTfChannelClosed(TfChannel *tfChannel, QTfChannel::Private *self);
    static void onSessionCreated(TfChannel *tfChannel, FsConference *conference,
                             FsParticipant *participant, QTfChannel::Private *self);
    static void onStreamCreated(TfChannel *tfChannel, TfStream *stream, QTfChannel::Private *self);
    static void onSrcPadAdded(TfStream *stream, GstPad *src, FsCodec *codec, QTfChannel::Private *self);
    static gboolean onRequestResource(TfStream *stream, guint direction, QTfChannel::Private *self);
    static void onFreeResource(TfStream *stream, guint direction, QTfChannel::Private *self);
    static GList *onStreamGetCodecConfig(TfChannel *channel, guint stream_id, guint media_type,
                                         guint direction, QTfChannel::Private *self);

    QString m_codecFileName;

private:
    TfChannel *m_tfChannel;
    QTfChannel *const q;
    QGstBusPtr m_bus;

    QList<QByteArray> m_audioSrcPads;
    QList<QByteArray> m_videoSrcPads;
    QGstElementPtr m_conference;

    QMutex m_srcPadAddedMutex;
    bool m_audioOutputDeviceIsOpen;
    bool m_videoOutputDeviceIsOpen;
    QGstPadPtr m_tmpAudioSrcPad;
    QGstPadPtr m_tmpVideoSrcPad;
};

QTfChannel::Private::~Private()
{
    g_object_unref(m_tfChannel);
}

void QTfChannel::Private::init(Tp::StreamedMediaChannelPtr channel, QGstBusPtr bus)
{
    m_tfChannel = Tp::createFarsightChannel(channel);
    Q_ASSERT(m_tfChannel);

    m_audioOutputDeviceIsOpen = false;
    m_videoOutputDeviceIsOpen = false;

    /* Set up the telepathy farsight channel */
    g_signal_connect(m_tfChannel, "closed",
                     G_CALLBACK(&QTfChannel::Private::onTfChannelClosed), this);
    g_signal_connect(m_tfChannel, "session-created",
                     G_CALLBACK(&QTfChannel::Private::onSessionCreated), this);
    g_signal_connect(m_tfChannel, "stream-created",
                     G_CALLBACK(&QTfChannel::Private::onStreamCreated), this);
    g_signal_connect(m_tfChannel, "stream-get-codec-config",
                     G_CALLBACK(&QTfChannel::Private::onStreamGetCodecConfig), this);

    m_bus = bus;
    connect(bus.data(), SIGNAL(message(GstMessage*)), q, SLOT(onBusMessage(GstMessage*)));

    connect(q, SIGNAL(audioSrcPadAdded(QtGstreamer::QGstPadPtr)),
            q, SLOT(onAudioSrcPadAdded(QtGstreamer::QGstPadPtr)));
    connect(q, SIGNAL(videoSrcPadAdded(QtGstreamer::QGstPadPtr)),
            q, SLOT(onVideoSrcPadAdded(QtGstreamer::QGstPadPtr)));

    qRegisterMetaType<QGstPadPtr>();
}

void QTfChannel::Private::onBusMessage(GstMessage *message)
{
    tf_channel_bus_message(m_tfChannel, message);
}

void QTfChannel::Private::onAudioSrcPadAdded(QGstPadPtr pad)
{
    m_audioSrcPads.append(pad->property<QByteArray>("name"));
}

void QTfChannel::Private::onVideoSrcPadAdded(QGstPadPtr pad)
{
    m_videoSrcPads.append(pad->property<QByteArray>("name"));
}

void QTfChannel::Private::onConferencePadRemoved(QGstPadPtr pad)
{
    if ( pad->direction() != QGstPad::Src ) {
        return;
    }

    QByteArray name = pad->property<QByteArray>("name");
    int index = -1;
    if ( (index = m_audioSrcPads.indexOf(name)) != -1 ) {
        m_audioSrcPads.removeAt(index);
        emit q->audioSrcPadRemoved(pad);
    } else if ( (index = m_videoSrcPads.indexOf(name)) != -1 ) {
        m_videoSrcPads.removeAt(index);
        emit q->videoSrcPadRemoved(pad);
    } else {
        Q_ASSERT(false);
    }
}

void QTfChannel::Private::onTfChannelClosed(TfChannel *tfChannel, QTfChannel::Private *self)
{
    Q_UNUSED(tfChannel);
    self->m_bus->removeSignalWatch();
    emit self->q->closed();
}

void QTfChannel::Private::onSessionCreated(TfChannel *tfChannel, FsConference *conference,
                                           FsParticipant *participant, QTfChannel::Private *self)
{
    Q_UNUSED(tfChannel);
    Q_UNUSED(participant);
    self->m_bus->addSignalWatch();
    self->m_conference = QGstElement::fromGstElement(GST_ELEMENT(conference));
    connect(self->m_conference.data(), SIGNAL(padRemoved(QtGstreamer::QGstPadPtr)),
            self->q, SLOT(onConferencePadRemoved(QtGstreamer::QGstPadPtr)));
    emit self->q->sessionCreated(self->m_conference);
}


void QTfChannel::Private::onStreamCreated(TfChannel *tfChannel, TfStream *stream,
                                          QTfChannel::Private *self)
{
    Q_UNUSED(tfChannel);
    g_signal_connect(stream, "src-pad-added",
                     G_CALLBACK(&QTfChannel::Private::onSrcPadAdded), self);
    g_signal_connect(stream, "request-resource",
                     G_CALLBACK(&QTfChannel::Private::onRequestResource), self);
    g_signal_connect(stream, "free-resource",
                     G_CALLBACK(&QTfChannel::Private::onFreeResource), self);
}

/* WARNING this method is called in a different thread. God knows why... !@*#&^*&^ */
void QTfChannel::Private::onSrcPadAdded(TfStream *stream, GstPad *src,
                                        FsCodec *codec, QTfChannel::Private *self)
{
    /* The tp-farsight api is broken, so this signal may come either before or after
       the request-resource signal, so that's why we have to use a mutex and the private
       bool value to remember which function was called first. */

    Q_UNUSED(codec);
    guint media_type;
    g_object_get(stream, "media-type", &media_type, NULL);

    QGstPadPtr pad = QGstPad::fromGstPad(src);
    QMutexLocker l(&self->m_srcPadAddedMutex);

    switch (media_type) {
    case Tp::MediaStreamTypeAudio:
        if ( self->m_audioOutputDeviceIsOpen ) {
            QMetaObject::invokeMethod(self->q, "audioSrcPadAdded", Qt::QueuedConnection,
                                      Q_ARG(QtGstreamer::QGstPadPtr, pad));
        } else {
            self->m_tmpAudioSrcPad = pad;
        }
        break;
    case Tp::MediaStreamTypeVideo:
        if ( self->m_videoOutputDeviceIsOpen ) {
            QMetaObject::invokeMethod(self->q, "videoSrcPadAdded", Qt::QueuedConnection,
                                      Q_ARG(QtGstreamer::QGstPadPtr, pad));
        } else {
            self->m_tmpVideoSrcPad = pad;
        }
        return;
    default:
        Q_ASSERT(false);
    }
}

gboolean QTfChannel::Private::onRequestResource(TfStream *stream, guint direction,
                                                QTfChannel::Private *self)
{
    guint media_type;
    g_object_get(stream, "media-type", &media_type, NULL);

    switch (media_type) {
    case Tp::MediaStreamTypeAudio:
    {
        switch(direction) {
        case Tp::MediaStreamDirectionSend:
        {
            GstPad *sink;
            g_object_get(stream, "sink-pad", &sink, NULL);
            QGstPadPtr sinkPad = QGstPad::fromGstPad(sink);
            gst_object_unref(sink);

            //FIXME here I assume that only one stream can add an audio sink pad.
            //The telepathy spec and the tp-farsight docs don't mention anything about that.
            bool success = false;
            emit self->q->openAudioInputDevice(&success);
            if ( success ) {
                emit self->q->audioSinkPadAdded(sinkPad);
            }
            return success;
        }
        case Tp::MediaStreamDirectionReceive:
        {
            /* The tp-farsight api is broken, so this signal may come either before or after
               the src-pad-added signal, so that's why we have to use a mutex and the private
               bool value to remember which function was called first. */

            QMutexLocker l(&self->m_srcPadAddedMutex);
            if ( !self->m_audioOutputDeviceIsOpen ) {
                bool success = false;
                emit self->q->openAudioOutputDevice(&success);
                self->m_audioOutputDeviceIsOpen = success;

                if ( success && !self->m_tmpAudioSrcPad.isNull() ) {
                    emit self->q->audioSrcPadAdded(self->m_tmpAudioSrcPad);
                    self->m_tmpAudioSrcPad = QGstPadPtr();
                }
                return success;
            } else {
                return true; //audio output device is already open
            }
        }
        default:
            Q_ASSERT(false);
        }
    }
    case Tp::MediaStreamTypeVideo:
    {
        switch(direction) {
        case Tp::MediaStreamDirectionSend:
        {
            GstPad *sink;
            g_object_get(stream, "sink-pad", &sink, NULL);
            QGstPadPtr sinkPad = QGstPad::fromGstPad(sink);
            gst_object_unref(sink);

            //FIXME here I assume that only one stream can add a video sink pad.
            //The telepathy spec and the tp-farsight docs don't mention anything about that.
            bool success = false;
            emit self->q->openVideoInputDevice(&success);
            if ( success ) {
                emit self->q->videoSinkPadAdded(sinkPad);
            }
            return success;
        }
        case Tp::MediaStreamDirectionReceive:
        {
            /* The tp-farsight api is broken, so this signal may come either before or after
               the src-pad-added signal, so that's why we have to use a mutex and the private
               bool value to remember which function was called first. */

            QMutexLocker l(&self->m_srcPadAddedMutex);
            if ( !self->m_videoOutputDeviceIsOpen ) {
                bool success = false;
                emit self->q->openVideoOutputDevice(&success);
                self->m_videoOutputDeviceIsOpen = success;

                if ( success && !self->m_tmpVideoSrcPad.isNull() ) {
                    emit self->q->videoSrcPadAdded(self->m_tmpVideoSrcPad);
                    self->m_tmpVideoSrcPad = QGstPadPtr();
                }
                return success;
            } else {
                return true; //video output device is already open
            }
        }
        default:
            Q_ASSERT(false);
        }
    }
    default:
        Q_ASSERT(false);
    }
    return false; //warnings--
}

void QTfChannel::Private::onFreeResource(TfStream *stream, guint direction,
                                         QTfChannel::Private *self)
{
    //the receive streams will be freed using the pad-removed signal on the conference.
    if ( direction == Tp::MediaStreamDirectionReceive ) {
        return;
    }

    guint media_type;
    g_object_get(stream, "media-type", &media_type, NULL);

    switch (media_type) {
    case Tp::MediaStreamTypeAudio:
        emit self->q->closeAudioInputDevice();
        break;
    case Tp::MediaStreamTypeVideo:
        emit self->q->closeVideoInputDevice();
        break;
    default:
        Q_ASSERT(false);
    }
}

GList *QTfChannel::Private::onStreamGetCodecConfig(TfChannel *channel, guint stream_id,
                                                   guint media_type, guint direction,
                                                   QTfChannel::Private *self)
{
    Q_UNUSED(channel);
    Q_UNUSED(stream_id);
    Q_UNUSED(media_type);
    Q_UNUSED(direction);
    Q_UNUSED(self);

    if ( !self->m_codecFileName.isEmpty() ) {
        return fs_codec_list_from_keyfile(QFile::encodeName(self->m_codecFileName), NULL);
    } else {
        return NULL;
    }
}


QTfChannel::QTfChannel(Tp::StreamedMediaChannelPtr channel, QGstBusPtr bus, QObject *parent)
    : QObject(parent), d(new Private(this))
{
    d->init(channel, bus);
}

QTfChannel::~QTfChannel()
{
    delete d;
}

void QTfChannel::setCodecsConfigFile(const QString & filename)
{
    d->m_codecFileName = filename;
}

#include "qtfchannel.moc"
