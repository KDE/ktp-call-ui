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
#ifndef KGSTDEVICES_VIDEOWIDGET_H
#define KGSTDEVICES_VIDEOWIDGET_H

#include "kgstdevices_export.h"
#include "../libqtgstreamer/qgstdeclarations.h"
#include <QtGui/QWidget>

namespace KGstDevices {

class AbstractRenderer;

/** This class is a QWidget that uses an AbstractRenderer to paint video
 * on it and also offers a GstBin with many elements to control various
 * properties of the video, like brightness, contrast, hue and saturation.
 * The video sink element of the renderer is added on the bin, so you should
 * use videoBin() to get the element where the video source should be
 * connected to.
 */
class KGSTDEVICES_EXPORT VideoWidget : public QWidget
{
    Q_OBJECT
    /** Controls the brightness of the video. Valid values are in the range [-1,1] */
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness)
    /** Controls the contrast of the video. Valid values are in the range [0,2] */
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast)
    /** Controls the hue of the video. Valid values are in the range [-1,1] */
    Q_PROPERTY(qreal hue READ hue WRITE setHue)
    /** Controls the saturation of the video. Valid values are in the range [0,2] */
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation)
public:
    /** Constructs a new VideoWidget that uses @a renderer to paint video and
     * has @a parent as a parent.
     */
    static VideoWidget *newVideoWidget(AbstractRenderer *renderer, QWidget *parent = 0);
    virtual ~VideoWidget();

    /** Returns the GstBin where the video source should be connected to. */
    QtGstreamer::QGstElementPtr videoBin() const;

    /** Returns a pointer to the renderer that is currently used to paint video. */
    AbstractRenderer *renderer() const;

    qreal brightness() const;
    void setBrightness(qreal brightness);

    qreal contrast() const;
    void setContrast(qreal contrast);

    qreal hue() const;
    void setHue(qreal hue);

    qreal saturation() const;
    void setSaturation(qreal saturation);

    /** Reimplemented from QWidget. @note The size hint depends on the movie
     * size that is set using AbstractRenderer::setMovieSize(). If no size has
     * been set on the renderer, the size hint is 640x480.
     */
    virtual QSize sizeHint() const;

private:
    struct Private;
    VideoWidget(Private *dd, QWidget *parent);
    Private *const d;
};

}

#endif
