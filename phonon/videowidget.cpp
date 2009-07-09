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

#include "videowidget.h"
#include <QtCore/QEvent>
#include <KDebug>
//#include "glrenderer.h"
//#include "widgetrenderer.h"
#include "x11renderer.h"

VideoWidget::VideoWidget(QWidget *parent) :
    QWidget(parent),
    m_videoBin(0),
    m_renderer(0),
    m_aspectRatio(AspectRatioAuto),
    m_brightness(0.0),
    m_hue(0.0),
    m_contrast(0.0),
    m_saturation(0.0),
    m_scaleMode(FitInView),
    m_videoBalance(0),
    m_colorspace(0),
    m_videoplug(0)
{
    setupVideoBin();
}

VideoWidget::~VideoWidget()
{
    if (m_videoBin) {
        gst_element_set_state (m_videoBin, GST_STATE_NULL);
        gst_object_unref (m_videoBin);
    }

    if (m_renderer)
        delete m_renderer;
}


void VideoWidget::setupVideoBin()
{

    m_renderer = new X11Renderer(this);
    GstElement *videoSink = m_renderer->videoSink();

    m_videoBin = gst_bin_new (NULL);
    Q_ASSERT(m_videoBin);
    gst_object_ref (GST_OBJECT (m_videoBin)); //Take ownership
    gst_object_sink (GST_OBJECT (m_videoBin));

    //The videoplug element is the final element before the pluggable videosink
    m_videoplug = gst_element_factory_make ("identity", NULL);

    //Colorspace ensures that the output of the stream matches the input format accepted by our video sink
    m_colorspace = gst_element_factory_make ("ffmpegcolorspace", NULL);

    //Video scale is used to prepare the correct aspect ratio and scale.
    GstElement *videoScale = gst_element_factory_make ("videoscale", NULL);

    //We need a queue to support the tee from parent node
    GstElement *queue = gst_element_factory_make ("queue", NULL);

    if (queue && m_videoBin && videoScale && m_colorspace && videoSink && m_videoplug) {
        //Ensure that the bare essentials are prepared
        gst_bin_add_many (GST_BIN (m_videoBin), queue, m_colorspace, m_videoplug, videoScale, videoSink, (const char*)NULL);
        bool success = false;
        //Video balance controls color/sat/hue in the YUV colorspace
        m_videoBalance = gst_element_factory_make ("videobalance", NULL);
        if (m_videoBalance) {
            // For video balance to work we have to first ensure that the video is in YUV colorspace,
            // then hand it off to the videobalance filter before finally converting it back to RGB.
            // Hence we nede a videoFilter to convert the colorspace before and after videobalance
            GstElement *m_colorspace2 = gst_element_factory_make ("ffmpegcolorspace", NULL);
            gst_bin_add_many(GST_BIN(m_videoBin), m_videoBalance, m_colorspace2, (const char*)NULL);
            success = gst_element_link_many(queue, m_colorspace, m_videoBalance, m_colorspace2, videoScale, m_videoplug, videoSink, (const char*)NULL);
        } else {
            //If video balance is not available, just connect to sink directly
            success = gst_element_link_many(queue, m_colorspace, videoScale, m_videoplug, videoSink, (const char*)NULL);
        }

        if (success) {
            GstPad *videopad = gst_element_get_pad (queue, "sink");
            gst_element_add_pad (m_videoBin, gst_ghost_pad_new ("sink", videopad));
            gst_object_unref (videopad);
            QWidget *parentWidget = qobject_cast<QWidget*>(parent());
            if (parentWidget)
                parentWidget->winId();  // Due to some existing issues with alien in 4.4,
                                        //  we must currently force the creation of a parent widget.
//FIXME            m_isValid = true; //initialization ok, accept input
        }
    }
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_ASSERT(m_renderer);
    m_renderer->handlePaint(event);
}

void VideoWidget::setVisible(bool val) {
    Q_ASSERT(m_renderer);
#if 0
    // Disable overlays for graphics view
    if (root() && window() && window()->testAttribute(Qt::WA_DontShowOnScreen) && !m_renderer->paintsOnWidget()) {
        kDebug() << "Widget rendering forced";
        GstElement *videoSink = m_renderer->videoSink();
        Q_ASSERT(videoSink);

        gst_element_set_state (videoSink, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(m_videoBin), videoSink);
        delete m_renderer;
        m_renderer = 0;

        // Use widgetRenderer as a fallback
        m_renderer = new WidgetRenderer(this);
        videoSink = m_renderer->videoSink();
        gst_bin_add(GST_BIN(m_videoBin), videoSink);
        gst_element_link(m_videoplug, videoSink);
        gst_element_set_state (videoSink, GST_STATE_PAUSED);

        // Request return to current state
        root()->invalidateGraph();
        root()->setState(root()->state());
    }
#endif
    QWidget::setVisible(val);    
}

bool VideoWidget::event(QEvent *event)
{
    if (m_renderer && m_renderer->eventFilter(event))
        return true;
    return QWidget::event(event);
}

VideoWidget::AspectRatio VideoWidget::aspectRatio() const
{
    return m_aspectRatio;
}

QSize VideoWidget::sizeHint() const
{
    if (!m_movieSize.isEmpty())
        return m_movieSize;
    else
        return QSize(640, 480);
}

void VideoWidget::setAspectRatio(AspectRatio aspectRatio)
{
    m_aspectRatio = aspectRatio;
    if (m_renderer)
        m_renderer->aspectRatioChanged(aspectRatio);
}

VideoWidget::ScaleMode VideoWidget::scaleMode() const
{
    return m_scaleMode;
}

QRect VideoWidget::scaleToAspect(QRect srcRect, int w, int h) const
{
    float width = srcRect.width();
    float height = srcRect.width() * (float(h) / float(w));
    if (height > srcRect.height()) {
        height = srcRect.height();
        width = srcRect.height() * (float(w) / float(h));
    }
    return QRect(0, 0, (int)width, (int)height);
}

/***
 * Calculates the actual rectangle the movie will be presented with
 **/
QRect VideoWidget::calculateDrawFrameRect() const
{
    QRect widgetRect = rect();
    QRect drawFrameRect;
    // Set m_drawFrameRect to be the size of the smallest possible
    // rect conforming to the aspect and containing the whole frame:
    switch (aspectRatio()) {

    case AspectRatioWidget:
        drawFrameRect = widgetRect;
        // No more calculations needed.
        return drawFrameRect;

    case AspectRatio4_3:
        drawFrameRect = scaleToAspect(widgetRect, 4, 3);
        break;

    case AspectRatio16_9:
        drawFrameRect = scaleToAspect(widgetRect, 16, 9);
        break;

    case AspectRatioAuto:
    default:
        drawFrameRect = QRect(0, 0, movieSize().width(), movieSize().height());
        break;
    }

    // Scale m_drawFrameRect to fill the widget
    // without breaking aspect:
    float widgetWidth = widgetRect.width();
    float widgetHeight = widgetRect.height();
    float frameWidth = widgetWidth;
    float frameHeight = drawFrameRect.height() * float(widgetWidth) / float(drawFrameRect.width());

    switch (scaleMode()) {
    case ScaleAndCrop:
        if (frameHeight < widgetHeight) {
            frameWidth *= float(widgetHeight) / float(frameHeight);
            frameHeight = widgetHeight;
        }
        break;
    case FitInView:
    default:
        if (frameHeight > widgetHeight) {
            frameWidth *= float(widgetHeight) / float(frameHeight);
            frameHeight = widgetHeight;
        }
        break;
    }
    drawFrameRect.setSize(QSize(int(frameWidth), int(frameHeight)));
    drawFrameRect.moveTo(int((widgetWidth - frameWidth) / 2.0f),
                           int((widgetHeight - frameHeight) / 2.0f));
    return drawFrameRect;
}

void VideoWidget::setScaleMode(ScaleMode scaleMode)
{
    m_scaleMode = scaleMode;
    if (m_renderer)
        m_renderer->scaleModeChanged(scaleMode);
}

qreal VideoWidget::brightness() const
{
    return m_brightness;
}

qreal clampedValue(qreal val)
{
    if (val > 1.0 )
        return 1.0;
    else if (val < -1.0)
        return -1.0;
    else return val;
}

void VideoWidget::setBrightness(qreal newValue)
{
    newValue = clampedValue(newValue);

    if (newValue == m_brightness)
        return;

    m_brightness = newValue;

    if (m_videoBalance)
        g_object_set(G_OBJECT(m_videoBalance), "brightness", newValue, (const char*)NULL); //gstreamer range is [-1, 1]

}

qreal VideoWidget::contrast() const
{
    return m_contrast;
}

void VideoWidget::setContrast(qreal newValue)
{
    newValue = clampedValue(newValue);

    if (newValue == m_contrast)
        return;

    m_contrast = newValue;

    if (m_videoBalance)
        g_object_set(G_OBJECT(m_videoBalance), "contrast", (newValue + 1.0), (const char*)NULL); //gstreamer range is [0-2]
}

qreal VideoWidget::hue() const
{
    return m_hue;
}

void VideoWidget::setHue(qreal newValue)
{
    if (newValue == m_hue)
        return;

    newValue = clampedValue(newValue);

    m_hue = newValue;

    if (m_videoBalance)
        g_object_set(G_OBJECT(m_videoBalance), "hue", newValue, (const char*)NULL); //gstreamer range is [-1, 1]
}

qreal VideoWidget::saturation() const
{
    return m_saturation;
}

void VideoWidget::setSaturation(qreal newValue)
{
    newValue = clampedValue(newValue);

    if (newValue == m_saturation)
        return;

    m_saturation = newValue;

    if (m_videoBalance)
        g_object_set(G_OBJECT(m_videoBalance), "saturation", newValue + 1.0, (const char*)NULL); //gstreamer range is [0, 2]
}


void VideoWidget::setMovieSize(const QSize &size)
{
    kDebug() << "New video size" << size;
    if (size == m_movieSize)
        return;
    m_movieSize = size;
    updateGeometry();
    update();

    if (m_renderer)
        m_renderer->movieSizeChanged(m_movieSize);
}

#if 0
void VideoWidget::mediaNodeEvent(const MediaNodeEvent *event)
{
    switch (event->type()) {
    case MediaNodeEvent::VideoSizeChanged: {
            const QSize *size = static_cast<const QSize*>(event->data());
            setMovieSize(*size);
        }
        break;
    default:
        break;
    }

    // Forward events to renderer
    if (m_renderer)
        m_renderer->handleMediaNodeEvent(event);
}
#endif

#include "videowidget.moc"
