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
class QTfChannel;
class ParticipantData;

class CallChannelHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    enum Who { Myself = 0, RemoteContact = 1 };

    inline CallChannelHandlerPrivate(CallChannelHandler *qq) : QObject(), q(qq) {}
    void init(const Tp::StreamedMediaChannelPtr & channel);

    inline Tp::StreamedMediaChannelPtr channel() const { return m_channel; }
    inline QList<CallParticipant*> participants() const { return m_participants.values(); }

private Q_SLOTS:
    void onChannelInvalidated(Tp::DBusProxy *proxy,
                              const QString & errorName,
                              const QString & errorMessage);

    void onSessionCreated(QGst::ElementPtr conference);
    void onQTfChannelClosed();

    void openAudioInputDevice(bool *success);
    void audioSinkPadAdded(QGst::PadPtr sinkPad);
    void closeAudioInputDevice();

    void openVideoInputDevice(bool *success);
    void videoSinkPadAdded(QGst::PadPtr sinkPad);
    void closeVideoInputDevice();

    void openAudioOutputDevice(bool *success);
    void audioSrcPadAdded(QGst::PadPtr srcPad);
    void closeAudioOutputDevice();

    void openVideoOutputDevice(bool *success);
    void videoSrcPadAdded(QGst::PadPtr srcPad);
    void closeVideoOutputDevice();

private:
    bool openInputDevice(bool audio);
    void sinkPadAdded(const QGst::PadPtr & sinkPad, bool audio);
    void closeInputDevice(bool audio);

    void srcPadAdded(QGst::PadPtr srcPad, bool audio);
    void closeOutputDevice(bool audio);

    void createAudioBin(QExplicitlySharedDataPointer<ParticipantData> & data);
    void createVideoBin(QExplicitlySharedDataPointer<ParticipantData> & data, bool withSink);

    // *** data ***

    CallChannelHandler *q;
    Tp::StreamedMediaChannelPtr m_channel;

    QMap<Who, CallParticipant*> m_participants;
    QExplicitlySharedDataPointer<ParticipantData> m_participantData[2];

    QTfChannel *m_qtfchannel;

    QGst::PipelinePtr m_pipeline;
    QGst::ElementPtr m_audioInputDevice;
    QGst::ElementPtr m_videoInputDevice;
    QGst::ElementPtr m_audioOutputDevice;
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
