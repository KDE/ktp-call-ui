/*
    Copyright (C) 2009-2010  George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#include <QGlib/Signal>
#include <QGst/Bus>
#include <QGst/Message>
#include <QtCore/QMutex>
#include <KDebug>

//BEGIN Ugly forward declarations

/* These declarations are copied here instead of including the proper headers,
 * because the proper headers also include too many unrelated stuff and we have
 * to depend on many useless (at build time) external libraries (such as dbus-glib,
 * libxml2, gstreamer) for no good reason.
 */

typedef unsigned long GType;
typedef int gboolean;
typedef char gchar;
typedef struct _GList GList;
typedef struct _TfChannel TfChannel;

extern "C" {
GType fs_codec_list_get_type (void);
GList *fs_codec_list_from_keyfile (const gchar *filename, GError **error);
gboolean tf_channel_bus_message(TfChannel *channel, GstMessage *message);
}

namespace Tp {
Q_DECL_IMPORT TfChannel *createFarsightChannel(const StreamedMediaChannelPtr &channel);
}
//END Ugly forward declarations


QGLIB_REGISTER_TYPE(GList*)
QGLIB_REGISTER_TYPE_IMPLEMENTATION(GList*, fs_codec_list_get_type())
QGLIB_REGISTER_VALUEIMPL_FOR_BOXED_TYPE(GList*)
Q_DECLARE_METATYPE(QGst::PadPtr)

class QTfChannel::Private
{
public:
    Private(QTfChannel *qq) : q(qq) {}

    void init(const Tp::StreamedMediaChannelPtr & channel, const QGst::BusPtr & bus);

    //GObject slots
    void onBusMessage(const QGst::MessagePtr & message);

    // -- from TfChannel signals
    void onTfChannelClosed();
    void onSessionCreated(const QGst::ElementPtr & conference);
    void onStreamCreated(const QGlib::ObjectPtr & stream);
    GList *onStreamGetCodecConfig();

    // -- from TfStream signals
    void onSrcPadAdded(const QGlib::ObjectPtr & stream, const QGst::PadPtr & src);
    bool onRequestResource(const QGlib::ObjectPtr & stream, uint direction);
    void onFreeResource(const QGlib::ObjectPtr & stream, uint direction);

    QString m_codecFileName;

private:
    QTfChannel *const q;

    QGlib::ObjectPtr m_tfChannel;
    QGst::BusPtr m_bus;
    QGst::ElementPtr m_conference;
};

void QTfChannel::Private::init(const Tp::StreamedMediaChannelPtr & channel, const QGst::BusPtr & bus)
{
    TfChannel *tfChannel = Tp::createFarsightChannel(channel);
    Q_ASSERT(tfChannel);
    m_tfChannel = QGlib::ObjectPtr::wrap(reinterpret_cast<GObject*>(tfChannel), false);

    /* Set up the telepathy farsight channel */
    QGlib::Signal::connect(m_tfChannel, "closed",
                           this, &QTfChannel::Private::onTfChannelClosed);
    QGlib::Signal::connect(m_tfChannel, "session-created",
                           this, &QTfChannel::Private::onSessionCreated);
    QGlib::Signal::connect(m_tfChannel, "stream-created",
                           this, &QTfChannel::Private::onStreamCreated);
    QGlib::Signal::connect(m_tfChannel, "stream-get-codec-config",
                           this, &QTfChannel::Private::onStreamGetCodecConfig);

    m_bus = bus;
    QGlib::Signal::connect(m_bus, "message", this, &QTfChannel::Private::onBusMessage);

    qRegisterMetaType<QGst::PadPtr>();
}

void QTfChannel::Private::onBusMessage(const QGst::MessagePtr & message)
{
    TfChannel *c = reinterpret_cast<TfChannel*>(static_cast<GObject*>(m_tfChannel));
    tf_channel_bus_message(c, message);
}

void QTfChannel::Private::onTfChannelClosed()
{
    m_bus->removeSignalWatch();
    Q_EMIT q->closed();
}

void QTfChannel::Private::onSessionCreated(const QGst::ElementPtr & conference)
{
    m_bus->addSignalWatch();
    m_conference = conference;
    Q_EMIT q->sessionCreated(m_conference);
}

void QTfChannel::Private::onStreamCreated(const QGlib::ObjectPtr & stream)
{
    QGlib::Signal::connect(stream, "src-pad-added", this,
                           &QTfChannel::Private::onSrcPadAdded, QGlib::Signal::PassSender);
    QGlib::Signal::connect(stream, "request-resource", this,
                           &QTfChannel::Private::onRequestResource, QGlib::Signal::PassSender);
    QGlib::Signal::connect(stream, "free-resource", this,
                           &QTfChannel::Private::onFreeResource, QGlib::Signal::PassSender);
}

GList *QTfChannel::Private::onStreamGetCodecConfig()
{
    if ( !m_codecFileName.isEmpty() ) {
        return fs_codec_list_from_keyfile(QFile::encodeName(m_codecFileName), NULL);
    } else {
        return NULL;
    }
}


/* WARNING this method is called in a different thread. */
void QTfChannel::Private::onSrcPadAdded(const QGlib::ObjectPtr & stream, const QGst::PadPtr & pad)
{
    switch (stream->property("media-type").get<uint>()) {
    case Tp::MediaStreamTypeAudio:
        Q_EMIT q->audioSrcPadAdded(pad);
        break;
    case Tp::MediaStreamTypeVideo:
        Q_EMIT q->videoSrcPadAdded(pad);
        break;
    default:
        Q_ASSERT(false);
    }
}

bool QTfChannel::Private::onRequestResource(const QGlib::ObjectPtr & stream, uint direction)
{
    switch (stream->property("media-type").get<uint>()) {
    case Tp::MediaStreamTypeAudio:
    {
        switch(direction) {
        case Tp::MediaStreamDirectionSend:
        {
            QGst::PadPtr sinkPad = stream->property("sink-pad").get<QGst::PadPtr>();

            bool success = false;
            Q_EMIT q->openAudioInputDevice(&success);
            if ( success ) {
                Q_EMIT q->audioSinkPadAdded(sinkPad);
            }
            return success;
        }
        case Tp::MediaStreamDirectionReceive:
        {
            bool success = false;
            Q_EMIT q->openAudioOutputDevice(&success);
            return success;
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
            QGst::PadPtr sinkPad = stream->property("sink-pad").get<QGst::PadPtr>();

            bool success = false;
            Q_EMIT q->openVideoInputDevice(&success);
            if ( success ) {
                Q_EMIT q->videoSinkPadAdded(sinkPad);
            }
            return success;
        }
        case Tp::MediaStreamDirectionReceive:
        {
            bool success = false;
            Q_EMIT q->openVideoOutputDevice(&success);
            return success;
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

void QTfChannel::Private::onFreeResource(const QGlib::ObjectPtr & stream, uint direction)
{
    switch (stream->property("media-type").get<uint>()) {
    case Tp::MediaStreamTypeAudio:
        switch(direction) {
        case Tp::MediaStreamDirectionReceive:
            Q_EMIT q->closeAudioOutputDevice();
            break;
        case Tp::MediaStreamDirectionSend:
            Q_EMIT q->closeAudioInputDevice();
            break;
        }
        break;
    case Tp::MediaStreamTypeVideo:
        switch(direction) {
        case Tp::MediaStreamDirectionReceive:
            Q_EMIT q->closeVideoOutputDevice();
            break;
        case Tp::MediaStreamDirectionSend:
            Q_EMIT q->closeVideoInputDevice();
            break;
        }
        break;
    default:
        Q_ASSERT(false);
    }
}


QTfChannel::QTfChannel(const Tp::StreamedMediaChannelPtr & channel,
                       const QGst::BusPtr & bus, QObject *parent)
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
