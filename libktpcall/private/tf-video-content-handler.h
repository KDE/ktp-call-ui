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


#ifndef TF_VIDEO_CONTENT_HANDLER_H
#define TF_VIDEO_CONTENT_HANDLER_H

#include "tf-content-handler.h"

namespace KTpCallPrivate {

class VideoSinkBin;

class TfVideoContentHandler : public TfContentHandler
{
    Q_OBJECT

public:
    TfVideoContentHandler(const QTf::ContentPtr & tfContent, TfChannelHandler *parent);
    virtual ~TfVideoContentHandler();

    void linkVideoPreviewSink(const QGst::ElementPtr & sink);
    void unlinkVideoPreviewSink();

    // TODO camera device control

    virtual BaseSinkController *createSinkController(const QGst::PadPtr & srcPad);
    virtual void releaseSinkControllerData(BaseSinkController *ctrl);

protected:
    virtual bool startSending();
    virtual void stopSending();

private:
    void createSrcBin(const QGst::ElementPtr & src);
    QGst::CapsPtr contentCaps() const;
    void onRestartSource();

    QGst::BinPtr m_srcBin;
    VideoSinkBin *m_videoPreviewBin;
};

} // KTpCallPrivate

#endif // TF_VIDEO_CONTENT_HANDLER_H
