/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _KGSTDEVICES_DEVICECHOOSER_P_H
#define _KGSTDEVICES_DEVICECHOOSER_P_H

#include "devicechooser.h"
#include "devicesmodel.h"
#include "ui_audiodevicechooser.h"
#include "../libqtgstreamer/qgstpipeline.h"
#include <QtCore/QPointer>

namespace KGstDevices {

struct DeviceChooserPrivate
{
    DeviceChooserPrivate(DeviceManager *m, DeviceManager::DeviceType t)
        : manager(m), type(t) {}

    QPointer<DeviceManager> manager;
    DevicesModel *model;
    DeviceManager::DeviceType type;
};

struct AudioDeviceChooserPrivate : public DeviceChooserPrivate
{
    AudioDeviceChooserPrivate(DeviceManager *m, DeviceManager::DeviceType t)
        : DeviceChooserPrivate(m, t) {}

    Ui::AudioDeviceChooser ui;
    bool showingDetails;
    bool testing;
    QtGstreamer::QGstPipelinePtr testPipeline;
};

class AudioDeviceChooser : public DeviceChooser
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(AudioDeviceChooser)
public:
    AudioDeviceChooser(DeviceManager *manager, DeviceManager::DeviceType type, QWidget *parent);
    virtual ~AudioDeviceChooser();

    virtual void setSelectedDevice(const Device & device);

private slots:
    void onSelectionChanged(const QItemSelection & selected);
    void onModelReset();
    void testDevice();
    void toggleDetails();
};

}

#endif
