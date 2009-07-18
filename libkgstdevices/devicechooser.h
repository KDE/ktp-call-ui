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
#ifndef _KGSTDEVICES_DEVICECHOOSER_H
#define _KGSTDEVICES_DEVICECHOOSER_H

#include "devicemanager.h"
#include <QtGui/QWidget>

namespace KGstDevices {

struct DeviceChooserPrivate;

class KGSTDEVICES_EXPORT DeviceChooser : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(DeviceChooser)
    Q_DECLARE_PRIVATE(DeviceChooser)
    Q_PROPERTY(Device selectedDevice READ selectedDevice WRITE setSelectedDevice USER true)
public:
    static DeviceChooser *newDeviceChooser(DeviceManager *manager, DeviceManager::DeviceType type,
                                           QWidget *parent = 0);
    virtual ~DeviceChooser();

    virtual Device selectedDevice() const;
    virtual void setSelectedDevice(const Device & device);

protected:
    DeviceChooser(DeviceChooserPrivate *dd, QWidget *parent);
    DeviceChooserPrivate *const d_ptr;
};

}

#endif
