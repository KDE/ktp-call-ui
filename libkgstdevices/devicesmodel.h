/*
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#ifndef KGSTDEVICES_DEVICESMODEL_H
#define KGSTDEVICES_DEVICESMODEL_H

#include "devicemanager.h"
#include <QtCore/QAbstractListModel>

namespace KGstDevices {

/** This class offers a read-only model that exports all the available devices
 * of a given type from a DeviceManager instance. To use it, call the populate()
 * function with the DeviceManager instance to be used and the type of the
 * devices to be listed as arguments.
 */
class KGSTDEVICES_EXPORT DevicesModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DevicesModel(QObject *parent = 0);
    virtual ~DevicesModel();

    void populate(DeviceManager *manager, DeviceManager::DeviceType type);

    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

private:
    struct Private;
    Private *const d;
    Q_PRIVATE_SLOT(d, void onDevicesChanged(DeviceManager::DeviceType type))
    Q_PRIVATE_SLOT(d, void onManagerDestroyed())
};

}

#endif
