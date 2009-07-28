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
#include "videowidget.h"
#include "abstractrenderer.h"
#include "../libqtgstreamer/qgstelementfactory.h"
#include "../libqtgstreamer/qgstbin.h"
#include "../libqtgstreamer/qgstghostpad.h"

using namespace QtGstreamer;

namespace KGstDevices {

struct VideoWidget::Private
{
    Private() : renderer(NULL) {}

    AbstractRenderer *renderer;
    QGstBinPtr videoBin;
    QGstElementPtr videoBalance;
};

VideoWidget::VideoWidget(Private *dd, QWidget *parent)
    : QWidget(parent), d(dd)
{
}

VideoWidget *VideoWidget::newVideoWidget(AbstractRenderer *renderer, QWidget *parent)
{
    Q_ASSERT(renderer);
    Private *d = new Private;

    d->renderer = renderer;
    d->videoBin = QGstBin::newBin();
    QGstElementPtr videoSink = renderer->videoSink();

    //We need a queue to support the tee from parent node
    QGstElementPtr queue = QGstElementFactory::make("queue");

    //Colorspace ensures that the output of the stream matches the input format accepted by our video sink
    QGstElementPtr colorSpace1 = QGstElementFactory::make("ffmpegcolorspace");

    //Video balance controls color/sat/hue in the YUV colorspace
    d->videoBalance = QGstElementFactory::make("videobalance");

    // For video balance to work we have to first ensure that the video is in YUV colorspace,
    // then hand it off to the videobalance filter before finally converting it back to RGB.
    // Hence we nede a videoFilter to convert the colorspace before and after videobalance
    QGstElementPtr colorSpace2 = QGstElementFactory::make("ffmpegcolorspace");

    //Video scale is used to prepare the correct aspect ratio and scale.
    QGstElementPtr videoScale = QGstElementFactory::make("videoscale");

    if ( !queue || !colorSpace1 || !d->videoBalance || !colorSpace2 || !videoScale || !videoSink ) {
        delete d;
        return NULL;
    }

    *(d->videoBin) << queue << colorSpace1 << d->videoBalance << colorSpace2 << videoScale << videoSink;
    QGstElement::link(queue, colorSpace1, d->videoBalance, colorSpace2, videoScale, videoSink);

    QGstPadPtr sink = queue->getStaticPad("sink");
    QGstPadPtr ghost = QGstGhostPad::newGhostPad("sink", sink);
    d->videoBin->addPad(ghost);

    if (parent) {
        // Due to some existing issues with alien in 4.4,
        // we must currently force the creation of a parent widget.
        parent->winId();
    }

    VideoWidget *videoWidget = new VideoWidget(d, parent);
    renderer->setParent(videoWidget);
    renderer->beginRendering(videoWidget);
    return videoWidget;
}

VideoWidget::~VideoWidget()
{
    d->videoBin->setState(QGstElement::Null);
    delete d->renderer;
}

QGstElementPtr VideoWidget::videoBin() const
{
    return d->videoBin;
}

AbstractRenderer *VideoWidget::renderer() const
{
    return d->renderer;
}

qreal VideoWidget::brightness() const
{
    return d->videoBalance->property<qreal>("brightness");
}

void VideoWidget::setBrightness(qreal newValue)
{
    newValue = qBound<qreal>(-1.0, newValue, 1.0); //gstreamer range is [-1, 1]
    d->videoBalance->setProperty("brightness", newValue);
}

qreal VideoWidget::contrast() const
{
    return d->videoBalance->property<qreal>("contrast");
}

void VideoWidget::setContrast(qreal newValue)
{
    newValue = qBound<qreal>(0.0, newValue, 2.0); //gstreamer range is [0, 2]
    d->videoBalance->setProperty("contrast", newValue);
}

qreal VideoWidget::hue() const
{
    return d->videoBalance->property<qreal>("hue");
}

void VideoWidget::setHue(qreal newValue)
{
    newValue = qBound<qreal>(-1.0, newValue, 1.0); //gstreamer range is [-1, 1]
    d->videoBalance->setProperty("hue", newValue);
}

qreal VideoWidget::saturation() const
{
    return d->videoBalance->property<qreal>("saturation");
}

void VideoWidget::setSaturation(qreal newValue)
{
    newValue = qBound<qreal>(0.0, newValue, 2.0); //gstreamer range is [0, 2]
    d->videoBalance->setProperty("saturation", newValue);
}

QSize VideoWidget::sizeHint() const
{
    if (!d->renderer->movieSize().isEmpty()) {
        return d->renderer->movieSize();
    } else {
        return QSize(640, 480);
    }
}

}

#include "videowidget.moc"
