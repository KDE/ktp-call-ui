/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _KGSTDEVICES_OVERLAYRENDERER_H
#define _KGSTDEVICES_OVERLAYRENDERER_H

#include "abstractrenderer.h"

namespace KGstDevices {

class OverlayWidget;

class KGSTDEVICES_EXPORT OverlayRenderer : public AbstractRenderer
{
public:
    OverlayRenderer(const QtGstreamer::QGstElementPtr & element, QObject *parent = 0);
    virtual ~OverlayRenderer();

    virtual void beginRendering(QWidget *videoWidget);

    virtual void setAspectRatio(AspectRatio aspectRatio);
    virtual void setScaleMode(ScaleMode scaleMode);
    virtual void setMovieSize(const QSize & size);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    void setOverlay();
    void windowExposed();

    OverlayWidget *m_renderWidget;
    friend class OverlayWidget;
};

}

#endif // _KGSTDEVICES_OVERLAYRENDERER_H
