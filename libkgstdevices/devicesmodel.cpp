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
#include "devicesmodel.h"
#include <QtCore/QPointer>
#include <KLocalizedString>

namespace KGstDevices {

struct DevicesModel::Private
{
    Private(DevicesModel *qq) : q(qq) {}

    void onDevicesChanged(DeviceManager::DeviceType type);
    void onManagerDestroyed();

    QPointer<DeviceManager> m_manager;
    DeviceManager::DeviceType m_type;

    DevicesModel *const q;
};

void DevicesModel::Private::onDevicesChanged(DeviceManager::DeviceType type)
{
    if (type == m_type) {
        q->reset();
    }
}

void DevicesModel::Private::onManagerDestroyed()
{
    q->reset();
}


DevicesModel::DevicesModel(QObject *parent)
    : QAbstractTableModel(parent), d(new Private(this))
{
}

DevicesModel::~DevicesModel()
{
    delete d;
}

void DevicesModel::populate(DeviceManager *manager, DeviceManager::DeviceType type)
{
    d->m_manager = manager;
    d->m_type = type;
    connect(manager, SIGNAL(devicesChanged(DeviceManager::DeviceType)),
            SLOT(onDevicesChanged(DeviceManager::DeviceType)));
    connect(manager, SIGNAL(destroyed()), SLOT(onManagerDestroyed()));
}

QVariant DevicesModel::data(const QModelIndex & index, int role) const
{
    if ( !index.isValid() || role != Qt::DisplayRole ) {
        return QVariant();
    }

    switch(index.column()) {
    case 0:
        return d->m_manager->availableDevices(d->m_type)[index.row()].name();
    case 1:
    {
        QByteArray driver = d->m_manager->availableDevices(d->m_type)[index.row()].driver();
        if ( driver.startsWith("auto") ) {
            return i18nc("autodetected audio or video system", "Autodetected");
        } else if ( driver.startsWith("alsa") ) {
            return i18n("ALSA");
        } else if ( driver.startsWith("oss") ) {
            return i18n("OSS");
        } else if ( driver.startsWith("jack") ) {
            return i18n("JACK");
        } else if ( driver.startsWith("pulse") ) {
            return i18n("PULSE");
        } else if ( driver.startsWith("v4l") ) {
            return i18n("Video4Linux");
        } else {
            return i18nc("Unknown audio or video system", "Unknown");
        }
    }
    case 2:
        return d->m_manager->availableDevices(d->m_type)[index.row()].driver();
    case 3:
        return d->m_manager->availableDevices(d->m_type)[index.row()].driverHandle();
    default:
        return QVariant();
    }
}

int DevicesModel::rowCount(const QModelIndex & parent) const
{
    if ( !parent.isValid() ) {
        if ( !d->m_manager ) {
            return 0;
        } else {
            return d->m_manager->availableDevices(d->m_type).size();
        }
    } else {
        return 0;
    }
}

int DevicesModel::columnCount(const QModelIndex & parent) const
{
    if ( !parent.isValid() ) {
        return 4;
    } else {
        return 0;
    }
}

QVariant DevicesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    switch(section) {
    case 0:
        return i18n("Device name");
    case 1:
        if ( d->m_type == DeviceManager::AudioInput || d->m_type == DeviceManager::AudioOutput ) {
            return i18n("Sound system");
        } else {
            return i18n("Video system");
        }
    case 2:
        return i18n("Gstreamer element");
    case 3:
        return i18n("Driver-specific handle");
    default:
        return QVariant();
    }
}

}

#include "devicesmodel.moc"
