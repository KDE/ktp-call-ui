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
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/StreamedMediaChannel>
class QTfChannel;
class DeviceManager;

class CallChannelHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    inline CallChannelHandlerPrivate(CallChannelHandler *qq) : QObject(), q(qq) {}

    void init(const Tp::StreamedMediaChannelPtr & channel);

    inline QHash<Tp::ContactPtr, CallParticipant*> participants() const { return m_participants; }

private Q_SLOTS:
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
    void audioSrcPadRemoved(QGst::PadPtr srcPad);

    void openVideoOutputDevice(bool *success);
    void videoSrcPadAdded(QGst::PadPtr srcPad);
    void videoSrcPadRemoved(QGst::PadPtr srcPad);

private:
    CallChannelHandler *q;
    Tp::StreamedMediaChannelPtr m_channel;
    QHash<Tp::ContactPtr, CallParticipant*> m_participants;

    QTfChannel *m_qtfchannel;
    DeviceManager *m_deviceManager;

    QGst::PipelinePtr m_pipeline;
    QGst::ElementPtr m_audioInputDevice;
    QGst::ElementPtr m_videoInputDevice;
    QGst::ElementPtr m_audioOutputDevice;
    QGst::ElementPtr m_audioOutputAdder;
    QHash<QByteArray, QGst::PadPtr> m_audioAdderPadsMap;
};

#endif
