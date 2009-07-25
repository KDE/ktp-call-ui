/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "videobalancewidget.h"
#include <QtCore/QVariant>
#include <QtGui/QSlider>
#include <QtGui/QFormLayout>
#include <KLocalizedString>

static QByteArray joinCamelCase(const char *prefix, const char *suffix)
{
    QByteArray result(prefix);
    result.append(QChar(suffix[0]).toUpper().toAscii());
    result.append(&suffix[1]);
    return result;
}

static const char *propertyNames[] = { "brightness", "contrast", "hue", "saturation" };

struct VideoBalanceWidget::Private
{
    QSlider *slider[4];
    QObject *control;
};

VideoBalanceWidget::VideoBalanceWidget(QWidget *parent)
    : QWidget(parent), d(new Private)
{
    QFormLayout *layout = new QFormLayout(this);

    for (int i = 0; i < 4; i++) {
        d->slider[i] = new QSlider(this);
        d->slider[i]->setOrientation(Qt::Horizontal);
        connect(d->slider[i], SIGNAL(valueChanged(int)), SLOT(onSliderValueChanged(int)));
    }

    layout->addRow(i18n("Brightness"), d->slider[0]);
    layout->addRow(i18n("Contrast"), d->slider[1]);
    layout->addRow(i18n("Hue"), d->slider[2]);
    layout->addRow(i18n("Saturation"), d->slider[3]);

    d->control = NULL;
    setEnabled(false);
}

VideoBalanceWidget::~VideoBalanceWidget()
{
    delete d;
}

void VideoBalanceWidget::setVideoBalanceControl(QObject *control)
{
    d->control = control;

    if ( d->control && d->control->property("brightness").isValid() ) {
        for (int i=0; i<4; i++) {
            QByteArray min = joinCamelCase("min", propertyNames[i]);
            QByteArray max = joinCamelCase("max", propertyNames[i]);

            d->slider[i]->setMinimum(d->control->property(min).toInt());
            d->slider[i]->setMaximum(d->control->property(max).toInt());
            d->slider[i]->setValue(d->control->property(propertyNames[i]).toInt());
            d->slider[i]->setTickPosition(QSlider::TicksAbove);
            d->slider[i]->setTickInterval((d->control->property(max).toInt() -
                                           d->control->property(min).toInt())/10);
        }
        setEnabled(true);
    } else {
        setEnabled(false);
    }
}

void VideoBalanceWidget::onSliderValueChanged(int value)
{
    Q_ASSERT(d->control);
    QSlider *slider = qobject_cast<QSlider*>(sender());

    for(int i=0; i<4; i++) {
        if ( slider == d->slider[i] ) {
            d->control->setProperty(propertyNames[i], value);
            slider->setValue(d->control->property(propertyNames[i]).toInt());
            return;
        }
    }
    Q_ASSERT(false); //execution should not reach here
}

#include "videobalancewidget.moc"
