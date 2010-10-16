/*
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

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
#ifndef CALLCHANNELHANDLER_P_H
#define CALLCHANNELHANDLER_P_H

#include "callchannelhandler.h"
#include <QGst/Pipeline>
#include <QGst/StreamVolume>
#include <QGst/ColorBalance>
#include <QGst/Ui/VideoWidget>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/StreamedMediaChannel>
class ParticipantData;


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


class CallChannelHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    enum Who { Myself = 0, RemoteContact = 1 };

    inline CallChannelHandlerPrivate(CallChannelHandler *qq) : QObject(), q(qq) {}
    void init(const Tp::StreamedMediaChannelPtr & channel);

    inline Tp::StreamedMediaChannelPtr channel() const { return m_channel; }
    inline QList<CallParticipant*> participants() const { return m_participants.values(); }
    Tp::ContactPtr contactOfParticipant(Who who) const;

private Q_SLOTS:
    void onChannelInvalidated(Tp::DBusProxy *proxy,
                              const QString & errorName,
                              const QString & errorMessage);

private:
    void onBusMessage(const QGst::MessagePtr & message);

    // -- from TfChannel signals
    void onTfChannelClosed();
    void onSessionCreated(QGst::ElementPtr & conference);
    void onStreamCreated(const QGlib::ObjectPtr & stream);
    GList *onStreamGetCodecConfig();

    // -- from TfStream signals
    void onSrcPadAdded(const QGlib::ObjectPtr & stream, QGst::PadPtr & src);
    bool onRequestResource(const QGlib::ObjectPtr & stream, uint direction);
    void onFreeResource(const QGlib::ObjectPtr & stream, uint direction);

    bool createAudioBin(QExplicitlySharedDataPointer<ParticipantData> & data);
    bool createVideoBin(QExplicitlySharedDataPointer<ParticipantData> & data, bool isVideoSrc);

    // *** data ***

    CallChannelHandler *q;
    Tp::StreamedMediaChannelPtr m_channel;

    QMap<Who, CallParticipant*> m_participants;
    QExplicitlySharedDataPointer<ParticipantData> m_participantData[2];

    QGst::PipelinePtr m_pipeline;
    QGst::ElementPtr m_audioInputDevice;
    QGst::ElementPtr m_videoInputDevice;
    QGst::ElementPtr m_audioOutputDevice;

    QGlib::ObjectPtr m_tfChannel;
    QGst::ElementPtr m_conference;
};

class ParticipantData : public QSharedData
{
public:
    inline ParticipantData() //just for being able to construct the array above without initialization
        : handlerPriv(NULL), who(CallChannelHandlerPrivate::Myself) {}
    inline ParticipantData(CallChannelHandlerPrivate *h, CallChannelHandlerPrivate::Who w)
        : handlerPriv(h), who(w) {}

    QGst::BinPtr audioBin;
    QGst::BinPtr videoBin;
    QGst::StreamVolumePtr volumeControl;
    QGst::ColorBalancePtr colorBalanceControl;
    QWeakPointer<QGst::Ui::VideoWidget> videoWidget;
    CallChannelHandlerPrivate *handlerPriv;
    CallChannelHandlerPrivate::Who who;
};

#endif
