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
#include "abstractrenderer.h"
#include "../libqtgstreamer/qgstelement.h"
#include <QtGui/QWidget>

namespace KGstDevices {

struct AbstractRenderer::Private
{
    QtGstreamer::QGstElementPtr videoSink;
    QWidget *widget;
    AspectRatio aspectRatio;
    ScaleMode scaleMode;
    QSize movieSize;
};

AbstractRenderer::AbstractRenderer(const QtGstreamer::QGstElementPtr & element, QObject *parent)
    : QObject(parent), d(new Private)
{
    d->videoSink = element;
}

AbstractRenderer::~AbstractRenderer()
{
    delete d;
}

QtGstreamer::QGstElementPtr AbstractRenderer::videoSink() const
{
    return d->videoSink;
}

AbstractRenderer::AspectRatio AbstractRenderer::aspectRatio() const
{
    return d->aspectRatio;
}

void AbstractRenderer::setAspectRatio(AspectRatio aspectRatio)
{
    d->aspectRatio = aspectRatio;
}

AbstractRenderer::ScaleMode AbstractRenderer::scaleMode() const
{
    return d->scaleMode;
}

void AbstractRenderer::setScaleMode(ScaleMode scaleMode)
{
    d->scaleMode = scaleMode;
}

QSize AbstractRenderer::movieSize() const
{
    return d->movieSize;
}

void AbstractRenderer::setMovieSize(const QSize & size)
{
    d->movieSize = size;
    if ( d->widget ) {
        d->widget->updateGeometry();
        d->widget->update();
    }
}

QWidget *AbstractRenderer::widget() const
{
    return d->widget;
}

void AbstractRenderer::setWidget(QWidget *widget)
{
    d->widget = widget;
}

static QRect scaleToAspect(const QRect & srcRect, int w, int h)
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
QRect AbstractRenderer::calculateDrawFrameRect() const
{
    QRect widgetRect = widget()->rect();
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

}
