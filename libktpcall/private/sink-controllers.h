/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
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
#ifndef SINK_CONTROLLERS_H
#define SINK_CONTROLLERS_H

#include "../volume-controller.h"
#include "video-sink-bin.h"
#include <TelepathyQt/Contact>
#include <QGst/Pipeline>
#include <QGst/Pad>

namespace KTpCallPrivate {

class BaseSinkController
{
public:
    virtual ~BaseSinkController() {}

    Tp::ContactPtr contact() const;

    virtual void initFromStreamingThread(const QGst::PadPtr & srcPad,
                                         const QGst::PipelinePtr & pipeline) = 0;
    virtual void initFromMainThread(const Tp::ContactPtr & contact);
    virtual void releaseFromStreamingThread(const QGst::PipelinePtr & pipeline);

protected:
    Tp::ContactPtr m_contact;
    QGst::BinPtr m_bin;
};


class AudioSinkController : public BaseSinkController
{
public:
    AudioSinkController(const QGst::PadPtr & adderRequestPad);
    virtual ~AudioSinkController();

    QGst::PadPtr adderRequestPad() const;
    VolumeController *volumeController() const;

    virtual void initFromStreamingThread(const QGst::PadPtr & srcPad,
                                         const QGst::PipelinePtr & pipeline);
    virtual void initFromMainThread(const Tp::ContactPtr & contact);
    virtual void releaseFromStreamingThread(const QGst::PipelinePtr & pipeline);

private:
    QGst::PadPtr m_adderRequestPad;
    VolumeController *m_volumeController;
};


class VideoSinkController : public BaseSinkController
{
public:
    VideoSinkController();
    virtual ~VideoSinkController();

    QGst::PadPtr requestSrcPad();
    void releaseSrcPad(const QGst::PadPtr & pad);

    void linkVideoSink(const QGst::ElementPtr & sink);
    void unlinkVideoSink();

    virtual void initFromStreamingThread(const QGst::PadPtr & srcPad,
                                         const QGst::PipelinePtr & pipeline);
    virtual void releaseFromStreamingThread(const QGst::PipelinePtr & pipeline);

private:
    //<ghost src pad, tee request src pad>
    QHash<QGst::PadPtr, QGst::PadPtr> m_pads;
    QGst::ElementPtr m_tee;
    uint m_padNameCounter;
    VideoSinkBin *m_videoSinkBin;
    QMutex m_videoSinkMutex;
};

} // KTpCallPrivate

Q_DECLARE_METATYPE(KTpCallPrivate::BaseSinkController*)

#endif //SINK_CONTROLLERS_H
