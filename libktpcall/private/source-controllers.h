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
#ifndef SOURCE_CONTROLLERS_H
#define SOURCE_CONTROLLERS_H

#include "../volume-controller.h"
#include "input-control-bin.h"
#include "video-sink-bin.h"

#include <QGst/Pipeline>
#include <QGst/Pad>

class BaseSourceController
{
public:
    explicit BaseSourceController(const QGst::PipelinePtr & pipeline);
    virtual ~BaseSourceController();

    QGst::PadPtr requestSrcPad();
    void releaseSrcPad(const QGst::PadPtr & pad);

    void connectToSource();
    void disconnectFromSource();

protected:
    virtual QGst::ElementPtr makeSilenceSource() = 0;
    virtual QGst::ElementPtr makeRealSource() = 0;
    virtual QGst::ElementPtr makeFilterBin() = 0;
    virtual void releaseRealSource() {}

    QGst::PipelinePtr m_pipeline;

private:
    void createBin();
    void destroyBin();

    QGst::ElementPtr m_source;
    InputControlBin *m_inputCtrlBin;
};


class AudioSourceController : public BaseSourceController
{
public:
    explicit AudioSourceController(const QGst::PipelinePtr & pipeline);
    virtual ~AudioSourceController();

    VolumeController *volumeController() const;
    //TODO level monitor

protected:
    virtual QGst::ElementPtr makeSilenceSource();
    virtual QGst::ElementPtr makeRealSource();
    virtual QGst::ElementPtr makeFilterBin();
    virtual void releaseRealSource();

private:
    VolumeController *m_volumeController;
};


class VideoSourceController : public BaseSourceController
{
public:
    explicit VideoSourceController(const QGst::PipelinePtr & pipeline);
    virtual ~VideoSourceController();

    void linkVideoPreviewSink(const QGst::ElementPtr & sink);
    void unlinkVideoPreviewSink();

protected:
    virtual QGst::ElementPtr makeSilenceSource();
    virtual QGst::ElementPtr makeRealSource();
    virtual QGst::ElementPtr makeFilterBin();

private:
    VideoSinkBin *m_videoSinkBin;
};

#endif // SOURCE_CONTROLLERS_H
