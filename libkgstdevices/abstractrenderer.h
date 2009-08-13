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

/** This is an abstract class that defines an interface and some basic functions
 * for a class that renders video from a gstreamer sink element on a QWidget.
 */
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

    /** Constructs an AbstractRenderer that uses @a element as a video sink
     * element and @a parent as a parent. @a element can be later retrieved
     * using the videoSink() function.
     */
    AbstractRenderer(const QtGstreamer::QGstElementPtr & element, QObject *parent = 0);
    virtual ~AbstractRenderer();

    /** Sets the widget where video should be painted on. This function must
     * be called for video rendering to begin. It should be called only once
     * in the beggining. If no widget is set, this will lead to undefined behavior
     * when the pipeline goes into PLAYING mode.
     */
    virtual void beginRendering(QWidget *videoWidget) = 0;

    /** Returns the gstreamer sink element where video data is retrieved from. */
    QtGstreamer::QGstElementPtr videoSink() const;

    /** Returns the aspect ratio that is currently set */
    AspectRatio aspectRatio() const;

    /** Sets the aspect ratio of the video */
    virtual void setAspectRatio(AspectRatio aspectRatio);

    /** Returns the scale mode that is currently set */
    ScaleMode scaleMode() const;

    /** Sets the scale mode of the video */
    virtual void setScaleMode(ScaleMode scaleMode);

    /** Returns the movie size that is currently set */
    QSize movieSize() const;

    /** Sets the size of the video. It is a good idea to set this, as it
     * will affect the aspect ratio (if aspectRatio is set to AspectRatioAuto)
     * and possibly the size hint, if the widget uses movieSize() as its size hint.
     */
    virtual void setMovieSize(const QSize & size);

protected:
    /** Returns a pointer to the widget where this renderer is drawing on. */
    QWidget *widget() const;

    /** Sets the widget where video will be drawn on. This function must be called
     * from the implementation of beginRendering()
     */
    void setWidget(QWidget *widget);

    /** Calculates the area of the widget where video should be drawn,
     * taking into account the movie size, the aspect ratio and the scale mode.
     */
    QRect calculateDrawFrameRect() const;

private:
    struct Private;
    Private *const d;
};

}

#endif //  _KGSTDEVICES_ABSTRACTRENDERER_H
