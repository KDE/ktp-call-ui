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
#include "overlayrenderer.h"
#include "../libqtgstreamer/interfaces/qgstxoverlay.h"
#include <QtGui/QWidget>
#include <QtGui/QPalette>
#include <QtGui/QApplication>
#include <QtGui/QPainter>

using namespace QtGstreamer;

namespace KGstDevices {

class OverlayWidget : public QWidget
{
public:
    OverlayWidget(QWidget *videoWidget, OverlayRenderer *renderer)
        : QWidget(videoWidget), m_videoWidget(videoWidget), m_renderer(renderer) {}

    virtual void paintEvent(QPaintEvent *event)
    {
        Q_UNUSED(event);
        QGstElement::State state = m_renderer->videoSink()->currentState();
        if (state == QGstElement::Playing || state == QGstElement::Paused) {
            m_renderer->windowExposed();
        } else {
            QPainter painter(this);
            painter.fillRect(m_videoWidget->rect(), m_videoWidget->palette().background());
        }
    }

private:
    QWidget *m_videoWidget;
    OverlayRenderer *m_renderer;
};

OverlayRenderer::OverlayRenderer(const QGstElementPtr & element, QObject *parent)
        : AbstractRenderer(element, parent),
          m_renderWidget(NULL)
{
}

OverlayRenderer::~OverlayRenderer()
{
    if ( m_renderWidget ) {
        m_renderWidget->setAttribute(Qt::WA_PaintOnScreen, false);
        m_renderWidget->setAttribute(Qt::WA_NoSystemBackground, false);
        delete m_renderWidget;
    }
}

void OverlayRenderer::beginRendering(QWidget *videoWidget)
{
    Q_ASSERT(!m_renderWidget && videoSink() && QGstXOverlay::isXOverlay(videoSink()));

    setWidget(videoWidget);
    m_renderWidget = new OverlayWidget(videoWidget, this);

    QPalette palette;
    palette.setColor(QPalette::Background, Qt::black);
    videoWidget->setPalette(palette);
    videoWidget->setAutoFillBackground(true);
    m_renderWidget->setMouseTracking(true);
    videoWidget->installEventFilter(this);

    m_renderWidget->setGeometry(calculateDrawFrameRect());
    setOverlay();
}

void OverlayRenderer::setAspectRatio(AspectRatio ratio)
{
    AbstractRenderer::setAspectRatio(ratio);
    if (m_renderWidget) {
        m_renderWidget->setGeometry(calculateDrawFrameRect());
    }
}

void OverlayRenderer::setScaleMode(ScaleMode scaleMode)
{
    AbstractRenderer::setScaleMode(scaleMode);
    if (m_renderWidget) {
        m_renderWidget->setGeometry(calculateDrawFrameRect());
    }
}

void OverlayRenderer::setMovieSize(const QSize & movieSize)
{
    AbstractRenderer::setMovieSize(movieSize);
    if (m_renderWidget) {
        m_renderWidget->setGeometry(calculateDrawFrameRect());
    }
}

bool OverlayRenderer::eventFilter(QObject *watched, QEvent *event)
{
    Q_ASSERT(watched == widget());

    if (event->type() == QEvent::Show) {
        // Setting these values ensures smooth resizing since it
        // will prevent the system from clearing the background
        m_renderWidget->setAttribute(Qt::WA_NoSystemBackground, true);
        m_renderWidget->setAttribute(Qt::WA_PaintOnScreen, true);
        setOverlay();
    } else if (event->type() == QEvent::Resize) {
        // This is a workaround for missing background repaints
        // when reducing window size
        m_renderWidget->setGeometry(calculateDrawFrameRect());
        windowExposed();
    } /*else if (event->type() == QEvent::Paint) {
        QPainter painter(widget());
        painter.fillRect(widget()->rect(), widget()->palette().background());
    }*/
    return false;
}
/*
void OverlayRenderer::handlePaint(QPaintEvent *)
{
    QPainter painter(m_videoWidget);
    painter.fillRect(m_videoWidget->rect(), m_videoWidget->palette().background());
}
*/

void OverlayRenderer::setOverlay()
{
    WId windowId = m_renderWidget->winId();
    // Even if we have created a winId at this point, other X applications
    // need to be aware of it.
    QApplication::syncX();
    QGstXOverlay(videoSink()).setXWindowId(windowId);

    windowExposed();
}

void OverlayRenderer::windowExposed()
{
    QApplication::syncX();
    QGstXOverlay(videoSink()).expose();
}

}
