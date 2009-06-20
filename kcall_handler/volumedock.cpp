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
#include "volumedock.h"
#include "volumewidget.h"
#include <QtGui/QHBoxLayout>

struct VolumeDock::Private
{
    VolumeWidget *inputVolumeWidget;
    VolumeWidget *outputVolumeWidget;
};

VolumeDock::VolumeDock(QWidget *parent)
    : QDockWidget(parent), d(new Private)
{
    QWidget *mainWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(mainWidget);
    d->inputVolumeWidget = new VolumeWidget(mainWidget);
    d->outputVolumeWidget = new VolumeWidget(mainWidget);
    layout->addWidget(d->inputVolumeWidget);
    layout->addWidget(d->outputVolumeWidget);
    setWidget(mainWidget);
}

VolumeDock::~VolumeDock()
{
    delete d;
}

VolumeWidget *VolumeDock::inputVolumeWidget() const
{
    return d->inputVolumeWidget;
}

VolumeWidget *VolumeDock::outputVolumeWidget() const
{
    return d->outputVolumeWidget;
}

#include "volumedock.moc"
