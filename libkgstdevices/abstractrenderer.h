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
#ifndef _KGSTDEVICES_ABSTRACTRENDERER_H
#define _KGSTDEVICES_ABSTRACTRENDERER_H

#include "kgstdevices_export.h"
#include "../libqtgstreamer/qgstdeclarations.h"
#include <QtCore/QObject>
class QWidget;
class QSize;
class QRect;

namespace KGstDevices {

class KGSTDEVICES_EXPORT AbstractRenderer : public QObject
{
    Q_OBJECT
public:
    enum AspectRatio {
        /**
         * Let the decoder find the aspect ratio automatically from the
         * media file (this is the default).
         */
        AspectRatioAuto = 0,
        /**
         * Fits the video into the widget making the aspect ratio depend
         * solely on the size of the widget. This way the aspect ratio
         * is freely resizeable by the user.
         */
        AspectRatioWidget = 1,
        /**
         * Make width/height == 4/3, which is the old TV size and
         * monitor size (1024/768 == 4/3). (4:3)
         */
        AspectRatio4_3 = 2,
        /**
         * Make width/height == 16/9, which is the size of most current
         * media. (16:9)
         */
        AspectRatio16_9 = 3
    };

    enum ScaleMode {
        FitInView = 0,
        ScaleAndCrop = 1
    };

    AbstractRenderer(const QtGstreamer::QGstElementPtr & element, QObject *parent = 0);
    virtual ~AbstractRenderer();

    virtual void beginRendering(QWidget *videoWidget) = 0;

    QtGstreamer::QGstElementPtr videoSink() const;

    AspectRatio aspectRatio() const;
    virtual void setAspectRatio(AspectRatio aspectRatio);

    ScaleMode scaleMode() const;
    virtual void setScaleMode(ScaleMode scaleMode);

    QSize movieSize() const;
    virtual void setMovieSize(const QSize & size);

protected:
    QWidget *widget() const;
    void setWidget(QWidget *widget);
    QRect calculateDrawFrameRect() const;

private:
    struct Private;
    Private *const d;
};

}

#endif //  _KGSTDEVICES_ABSTRACTRENDERER_H
