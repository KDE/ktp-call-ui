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
#ifndef _KGSTDEVICES_VIDEOWIDGET_H
#define _KGSTDEVICES_VIDEOWIDGET_H

#include "kgstdevices_export.h"
#include "../libqtgstreamer/qgstdeclarations.h"
#include <QtGui/QWidget>

namespace KGstDevices {

class AbstractRenderer;

class KGSTDEVICES_EXPORT VideoWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness)
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast)
    Q_PROPERTY(qreal hue READ hue WRITE setHue)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation)
public:
    static VideoWidget *newVideoWidget(AbstractRenderer *renderer, QWidget *parent = 0);
    virtual ~VideoWidget();

    QtGstreamer::QGstElementPtr videoBin() const;
    AbstractRenderer *renderer() const;

    qreal brightness() const;
    void setBrightness(qreal brightness);

    qreal contrast() const;
    void setContrast(qreal contrast);

    qreal hue() const;
    void setHue(qreal hue);

    qreal saturation() const;
    void setSaturation(qreal saturation);

    virtual QSize sizeHint() const;

private:
    struct Private;
    VideoWidget(Private *dd, QWidget *parent);
    Private *const d;
};

}

#endif
