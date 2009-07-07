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
#include "../libkgstdevices/volumecontrolinterface.h"
#include <QtGui/QSlider>
#include <QtGui/QVBoxLayout>

struct VolumeWidget::Private
{
    QSlider *slider;
    VolumeControlInterface *control;
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

void VolumeWidget::setVolumeControl(VolumeControlInterface *control)
{
    d->control = control;

    if ( d->control && d->control->volumeControlIsAvailable() ) {
        d->slider->setMinimum(d->control->minVolume());
        d->slider->setMaximum(d->control->maxVolume());
        d->slider->setValue(d->control->volume());
        d->slider->setTickPosition(QSlider::TicksAbove);
        d->slider->setTickInterval((d->control->maxVolume() - d->control->minVolume())/10);
        setEnabled(true);
    } else {
        setEnabled(false);
    }
}

void VolumeWidget::onSliderValueChanged(int value)
{
    Q_ASSERT(d->control);
    d->control->setVolume(value);
    d->slider->setValue(d->control->volume());
}

#include "volumewidget.moc"
