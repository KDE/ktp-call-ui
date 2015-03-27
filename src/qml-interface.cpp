/*
 *  Copyright (C) 2014 Ekaitz Zárraga <ekaitz.zarraga@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <KActionCollection>
#include <KDeclarative/KDeclarative>

#include <QGst/ElementFactory>
#include <QGst/Quick/VideoSurface>
#include <QGst/Init>

#include <QQmlContext>
#include <QQuickItem>
#include <QGraphicsObject>
#include <QStandardPaths>

#include "qml-interface.h"
#include "call-window.h"

struct QmlInterface::Private
{
    /*! Manages the video preview player*/
    QGst::Quick::VideoSurface *surfacePreview;
    /*! Manages the main video player*/
    QGst::Quick::VideoSurface *surface;

    QGst::ElementPtr videoSink;
    QGst::ElementPtr previewVideoSink;

    KDeclarative::KDeclarative kd;
};


QmlInterface::QmlInterface(CallWindow *parent)
    : QQuickView(), d(new Private)
{
    d->kd.setDeclarativeEngine(engine());
    d->kd.setupBindings();

    d->surface = new QGst::Quick::VideoSurface();
    rootContext()->setContextProperty(QLatin1String("videoSurface"), d->surface);

    d->surfacePreview = new QGst::Quick::VideoSurface();
    rootContext()->setContextProperty(QLatin1String("videoPreviewSurface"), d->surfacePreview);

    /* Store the video sinks here. Otherwise, no image is shown... */
    d->videoSink = d->surface->videoSink();
    d->previewVideoSink = d->surfacePreview->videoSink();

    setResizeMode(QQuickView::SizeRootObjectToView);

    rootContext()->setContextProperty("showMyVideoAction", parent->actionCollection()->action("showMyVideo"));
    rootContext()->setContextProperty("showDtmfAction", parent->actionCollection()->action("showDtmf"));
    rootContext()->setContextProperty("muteAction", parent->actionCollection()->action("mute"));

    setSource(QUrl(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("ktp-call-ui/Main.qml"))));
}

void QmlInterface::setLabel(const QString &name, const QString &imageUrl)
{
    QMetaObject::invokeMethod(rootObject(), "setLabel", Q_ARG(QVariant, name), Q_ARG(QVariant, imageUrl));
}

QGst::ElementPtr QmlInterface::getVideoSink()
{
    return d->videoSink;
}

QGst::ElementPtr QmlInterface::getVideoPreviewSink()
{
    return d->previewVideoSink;
}
void QmlInterface::setShowVideo(bool show)
{
    QMetaObject::invokeMethod(rootObject(), "showVideo", Q_ARG(QVariant, show));
}
void QmlInterface::setChangeHoldIcon(const QString &icon)
{
    QMetaObject::invokeMethod(rootObject(), "changeHoldIcon", Q_ARG(QVariant, icon));
}

void QmlInterface::setHoldEnabled(bool enable)
{
    QMetaObject::invokeMethod(rootObject(), "setHoldEnabled", Q_ARG(QVariant, enable));
}

QmlInterface::~QmlInterface()
{
    delete d;
}
