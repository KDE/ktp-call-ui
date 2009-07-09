/*  This file is part of the KDE project.

    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).

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

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QtGui/QWidget>
#include <gst/gst.h>
class AbstractRenderer;

class VideoWidget : public QWidget
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

    VideoWidget(QWidget *parent = 0);
    ~VideoWidget();

    void setupVideoBin();
    void paintEvent(QPaintEvent *event);
    void setVisible(bool);

    AspectRatio aspectRatio() const;
    void setAspectRatio(AspectRatio aspectRatio);
    ScaleMode scaleMode() const;
    void setScaleMode(ScaleMode);
    qreal brightness() const;
    void setBrightness(qreal);
    qreal contrast() const;
    void setContrast(qreal);
    qreal hue() const;
    void setHue(qreal);
    qreal saturation() const;
    void setSaturation(qreal);
    void setMovieSize(const QSize &size);
    QSize sizeHint() const;
    QRect scaleToAspect(QRect srcRect, int w, int h) const;
    QRect calculateDrawFrameRect() const;

    GstElement *videoElement()
    {
        Q_ASSERT(m_videoBin);
        return m_videoBin;
    }

    QSize movieSize() const {
        return m_movieSize;
    }

    bool event(QEvent *);


protected:
    GstElement *m_videoBin;
    QSize m_movieSize;
    AbstractRenderer *m_renderer;

private:
    AspectRatio m_aspectRatio;
    qreal m_brightness, m_hue, m_contrast, m_saturation;
    ScaleMode m_scaleMode;

    GstElement *m_videoBalance;
    GstElement *m_colorspace;
    GstElement *m_videoplug;
};

#endif // VIDEOWIDGET_H
