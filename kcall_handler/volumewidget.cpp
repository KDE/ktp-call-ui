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
#include "abstractmediahandler.h"
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtGui/QVBoxLayout>

struct VolumeWidget::Private
{
    QLabel *label;
    QSlider *slider;
    AbstractAudioDevice *device;
};

VolumeWidget::VolumeWidget(QWidget *parent)
    : QWidget(parent), d(new Private)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    d->label = new QLabel(this);
    d->slider = new QSlider(this);
    layout->addWidget(d->label);
    layout->addWidget(d->slider);

    d->slider->setMinimum(0);
    d->slider->setMaximum(100);
    d->slider->setOrientation(Qt::Horizontal);
    connect(d->slider, SIGNAL(valueChanged(int)), SLOT(onSliderValueChanged(int)));

    d->device = NULL;
    setEnabled(false);
}

VolumeWidget::~VolumeWidget()
{
    delete d;
}

void VolumeWidget::setAudioDevice(AbstractAudioDevice *device)
{
    d->device = device;

    if ( d->device ) {
        d->label->setText(device->name());
        d->slider->setValue(static_cast<int>(d->device->volume()*10));
        setEnabled(true);
    } else {
        setEnabled(false);
    }
}

void VolumeWidget::onSliderValueChanged(int value)
{
    Q_ASSERT(d->device);
    d->device->setVolume(qreal(value)/qreal(10));
    d->slider->setValue(static_cast<int>(d->device->volume()*10));
}

#include "volumewidget.moc"
