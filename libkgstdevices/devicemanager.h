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
#ifndef _KGSTDEVICES_DEVICEMANAGER_H
#define _KGSTDEVICES_DEVICEMANAGER_H

#include "kgstdevices_export.h"
#include "../libqtgstreamer/qgstelement.h"
#include <QtCore/QSharedDataPointer>
class KConfigGroup;
class QWidget;

namespace KGstDevices {

class Device;
class VideoWidget;

struct DeviceManagerPrivate;
struct DevicePrivate;

class KGSTDEVICES_EXPORT DeviceManager : public QObject
{
    Q_OBJECT
    Q_ENUMS(DeviceType)
    Q_DISABLE_COPY(DeviceManager)
public:
    enum DeviceType { AudioInput, AudioOutput, VideoInput, VideoOutput };

    explicit DeviceManager(QObject *parent = 0);
    virtual ~DeviceManager();

    bool showsTestDevices() const;
    void setShowTestDevices(bool show);

    QList<Device> availableDevices(DeviceType type) const;

    Device currentDeviceForType(DeviceType type) const;
    void setCurrentDeviceForType(DeviceType type, const Device & device);

    void loadDefaults();
    void loadConfig(const KConfigGroup & config);
    void saveConfig(KConfigGroup config) const;

    QtGstreamer::QGstElementPtr newAudioInputElement();
    QtGstreamer::QGstElementPtr newAudioOutputElement();
    QtGstreamer::QGstElementPtr newVideoInputElement();
    VideoWidget *newVideoWidget(QWidget *parent = 0);

Q_SIGNALS:
    void devicesChanged(DeviceManager::DeviceType type);
    void currentDeviceChanged(DeviceManager::DeviceType type);

private:
    friend struct DeviceManagerPrivate;
    DeviceManagerPrivate *const d;
    Q_PRIVATE_SLOT(d, void onDeviceAdded(const QString & udi))
    Q_PRIVATE_SLOT(d, void onDeviceRemoved(const QString & udi))
};

class KGSTDEVICES_EXPORT Device
{
public:
    Device(const Device & other);
    virtual ~Device();

    Device & operator=(const Device & other);
    bool operator==(const Device & other) const;
    bool isValid() const;

    QString name() const;
    QByteArray driver() const;
    QVariant driverHandle() const;

private:
    //for internal use from DeviceManagerPrivate
    Device();
    Device(const DevicePrivate & dd);
    QString udi() const;
    friend struct DeviceManagerPrivate;
    friend QDataStream & operator>>(QDataStream & stream, Device & device);

    QSharedDataPointer<DevicePrivate> d;
};

}

#endif
