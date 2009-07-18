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
#include "volumewidget.h"
#include <QtCore/QVariant>
#include <QtGui/QSlider>
#include <QtGui/QVBoxLayout>

struct VolumeWidget::Private
{
    QSlider *slider;
    QObject *control;
};

VolumeWidget::VolumeWidget(QWidget *parent)
    : QWidget(parent), d(new Private)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    d->slider = new QSlider(this);
    layout->addWidget(d->slider);

    d->slider->setOrientation(Qt::Horizontal);
    connect(d->slider, SIGNAL(valueChanged(int)), SLOT(onSliderValueChanged(int)));

    d->control = NULL;
    setEnabled(false);
}

VolumeWidget::~VolumeWidget()
{
    delete d;
}

void VolumeWidget::setVolumeControl(QObject *control)
{
    d->control = control;

    if ( d->control && d->control->property("volume").isValid() ) {
        d->slider->setMinimum(d->control->property("minVolume").toInt());
        d->slider->setMaximum(d->control->property("maxVolume").toInt());
        d->slider->setValue(d->control->property("volume").toInt());
        d->slider->setTickPosition(QSlider::TicksAbove);
        d->slider->setTickInterval((d->control->property("maxVolume").toInt() -
                                    d->control->property("minVolume").toInt())/10);
        setEnabled(true);
    } else {
        setEnabled(false);
    }
}

void VolumeWidget::onSliderValueChanged(int value)
{
    Q_ASSERT(d->control);
    d->control->setProperty("volume", QVariant(value));
    d->slider->setValue(d->control->property("volume").toInt());
}

#include "volumewidget.moc"
