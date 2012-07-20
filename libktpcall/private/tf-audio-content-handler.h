/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
    Copyright (C) 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TF_AUDIO_CONTENT_HANDLER_H
#define TF_AUDIO_CONTENT_HANDLER_H

#include "tf-content-handler.h"

class VolumeController;

namespace KTpCallPrivate {

class TfAudioContentHandler : public TfContentHandler
{
    Q_OBJECT
public:
    TfAudioContentHandler(const QTf::ContentPtr& tfContent, TfChannelHandler* parent);
    virtual ~TfAudioContentHandler();

    VolumeController *inputVolumeController() const;
    VolumeController *outputVolumeController() const;

    // TODO audio device control

    virtual BaseSinkController *createSinkController(const QGst::PadPtr & srcPad);
    virtual void releaseSinkControllerData(BaseSinkController *ctrl);

protected:
    virtual bool startSending();
    virtual void stopSending();

private Q_SLOTS:
    void onSinkCreated();
    void onSinkDestroyed();

private:
    void refSink();
    void unrefSink();

    QMutex m_mutex;
    QGst::ElementPtr m_sink;
    QGst::ElementPtr m_outputVolume;
    QGst::ElementPtr m_outputAdder;
    int m_sinkRefCount;

    QGst::BinPtr m_srcBin;

    VolumeController *m_inputVolumeController;
    VolumeController *m_outputVolumeController;
};

} // KTpCallPrivate

#endif // TF_AUDIO_CONTENT_HANDLER_H
