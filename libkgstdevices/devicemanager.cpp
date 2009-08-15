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
#include "devicemanager.h"
#include "overlayrenderer.h"
#include "videowidget.h"
#include "../libqtgstreamer/qgstelementfactory.h"
#include <QtCore/QPointer>
#include <QtCore/QDataStream>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KDebug>
#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/AudioInterface>
#include <solid/video.h>
#include <kdeversion.h>

namespace KGstDevices {

//BEGIN Device

struct DevicePrivate : public QSharedData
{
    QString name;
    QByteArray driver;
    QVariant driverHandle;
    QString udi;
};

Device::Device() : d(new DevicePrivate) {}
Device::Device(const DevicePrivate & dd) : d(new DevicePrivate(dd)) {}
Device::Device(const Device & other) : d(other.d) {}
Device::~Device() {}

Device & Device::operator=(const Device & other)
{
    d = other.d;
    return *this;
}

bool Device::operator==(const Device & other) const
{
    return (d->driver == other.d->driver &&
            d->driverHandle == other.d->driverHandle);
}

bool Device::isValid() const
{
    return (!d->driver.isEmpty());
}

QString Device::name() const
{
    return d->name;
}

QByteArray Device::driver() const
{
    return d->driver;
}

QVariant Device::driverHandle() const
{
    return d->driverHandle;
}

QString Device::udi() const
{
    return d->udi;
}

QDataStream & operator<<(QDataStream & stream, const Device & device)
{
    stream << device.driver() << device.driverHandle();
    return stream;
}

QDataStream & operator>>(QDataStream & stream, Device & device)
{
    stream >> device.d->driver;
    stream >> device.d->driverHandle;
    return stream;
}

//END Device

//BEGIN DeviceManager

struct DeviceManagerPrivate
{
    DeviceManagerPrivate(DeviceManager *qq) : q(qq), m_showTestDevices(false) {}

    void detectDevices();
    void addDevice(const Solid::Device & sd);
    void onDeviceAdded(const QString & udi);
    void onDeviceRemoved(const QString & udi);
    void addTestDevices();
    void removeTestDevices();

    QList<Device> m_availableDevices[4];
    Device m_currentDevices[4];

    DeviceManager *const q;
    bool m_showTestDevices;
};

void DeviceManagerPrivate::detectDevices()
{
    //discover audio devices
    QList<Solid::Device> solidAudioDevices =
                    Solid::Device::listFromType(Solid::AudioInterface::deviceInterfaceType());
    foreach(const Solid::Device & solidDevice, solidAudioDevices) {
        addDevice(solidDevice);
    }

    //discover video input devices
    QList<Solid::Device> solidVideoDevices =
                    Solid::Device::listFromType(Solid::Video::deviceInterfaceType());
    foreach(const Solid::Device & solidDevice, solidVideoDevices) {
        addDevice(solidDevice);
    }

    //append video output devices
    {
        DevicePrivate d;
#ifdef Q_WS_X11
        d.name = i18n("Xv video renderer");
        d.driver = "xvimagesink";
        m_availableDevices[DeviceManager::VideoOutput].append(Device(d));

        d.name = i18n("X11 video renderer");
        d.driver = "ximagesink";
        m_availableDevices[DeviceManager::VideoOutput].append(Device(d));
#endif
    }

    //prepend the default devices for each category
    {
        DevicePrivate d;
        d.name = i18n("Autodetect");

        d.driver = "autoaudiosrc";
        m_availableDevices[DeviceManager::AudioInput].prepend(Device(d));

        d.driver = "autoaudiosink";
        m_availableDevices[DeviceManager::AudioOutput].prepend(Device(d));

        d.driver = "autovideosrc";
        m_availableDevices[DeviceManager::VideoInput].prepend(Device(d));
    }

#ifdef USE_TEST_DEVICES
    //and append the test devices, for testing purposes only
    addTestDevices();
#endif

    //set default devices
    q->loadDefaults();
}

static QByteArray formatAlsaDevice(const QByteArray & plugin, const QVariantList & list)
{
    QByteArray result = plugin;
    result.append(':');
    result.append(list[0].toByteArray());
    if ( list.size() > 1 && list[1].isValid() ) {
        result.append(',');
        result.append(list[1].toByteArray());
        if ( list.size() > 2 && list[2].isValid() ) {
            result.append(',');
            result.append(list[2].toByteArray());
        }
    }
    return result;
}

void DeviceManagerPrivate::addDevice(const Solid::Device & solidDevice)
{
    if ( solidDevice.is<Solid::AudioInterface>() ) {
        const Solid::AudioInterface *solidAudioDevice = solidDevice.as<Solid::AudioInterface>();

        if ( solidAudioDevice->deviceType() & Solid::AudioInterface::AudioInput )
        {
            Solid::Device vendorDevice = solidDevice;
            while ( vendorDevice.isValid() && vendorDevice.vendor().isEmpty() ) {
                vendorDevice = Solid::Device(vendorDevice.parentUdi());
            }
            DevicePrivate device;
            device.name = QString("%1 - %2").arg(vendorDevice.vendor(), solidDevice.product());
            device.udi = solidDevice.udi();

            switch ( solidAudioDevice->driver() ) {
            case Solid::AudioInterface::Alsa:
            {
                device.driver = "alsasrc";
                device.driverHandle = formatAlsaDevice("hw", solidAudioDevice->driverHandle().toList());
                m_availableDevices[DeviceManager::AudioInput].append(Device(device));
                emit q->devicesChanged(DeviceManager::AudioInput);
                break;
            }
            case Solid::AudioInterface::OpenSoundSystem:
                device.driver = "osssrc";
                device.driverHandle = solidAudioDevice->driverHandle();
                m_availableDevices[DeviceManager::AudioInput].append(Device(device));
                emit q->devicesChanged(DeviceManager::AudioInput);
                break;
            default:
                break; //skip the device
            }
        }

        if ( solidAudioDevice->deviceType() & Solid::AudioInterface::AudioOutput )
        {
            Solid::Device vendorDevice = solidDevice;
            while ( vendorDevice.isValid() && vendorDevice.vendor().isEmpty() ) {
                vendorDevice = Solid::Device(vendorDevice.parentUdi());
            }
            DevicePrivate device;
            device.name = QString("%1 - %2").arg(vendorDevice.vendor(), solidDevice.product());
            device.udi = solidDevice.udi();

            switch ( solidAudioDevice->driver() ) {
            case Solid::AudioInterface::Alsa:
            {
                device.driver = "alsasink";
                device.driverHandle = formatAlsaDevice("dmix", solidAudioDevice->driverHandle().toList());
                m_availableDevices[DeviceManager::AudioOutput].append(Device(device));
                emit q->devicesChanged(DeviceManager::AudioOutput);
                break;
            }
            case Solid::AudioInterface::OpenSoundSystem:
                device.driver = "osssink";
                device.driverHandle = solidAudioDevice->driverHandle();
                m_availableDevices[DeviceManager::AudioOutput].append(Device(device));
                emit q->devicesChanged(DeviceManager::AudioOutput);
                break;
            default:
                break; //skip the device
            }
        }
    } else if ( solidDevice.is<Solid::Video>() ) {
        const Solid::Video *solidVideoDevice = solidDevice.as<Solid::Video>();

        if ( solidVideoDevice->supportedProtocols().contains("video4linux") ) {
            Solid::Device vendorDevice = solidDevice;
            while ( vendorDevice.isValid() && vendorDevice.vendor().isEmpty() ) {
                vendorDevice = Solid::Device(vendorDevice.parentUdi());
            }
            DevicePrivate device;
            device.name = QString("%1 - %2").arg(vendorDevice.vendor(), solidDevice.product());
            device.udi = solidDevice.udi();
            if ( solidVideoDevice->supportedDrivers("video4linux").contains("video4linux2") ) {
                device.driver = "v4l2src";
                device.driverHandle = solidVideoDevice->driverHandle("video4linux2");
                m_availableDevices[DeviceManager::VideoInput].append(Device(device));
                emit q->devicesChanged(DeviceManager::VideoInput);
            } else if ( solidVideoDevice->supportedDrivers("video4linux").contains("video4linux") ) {
                device.driver = "v4lsrc";
                device.driverHandle = solidVideoDevice->driverHandle("video4linux");
                m_availableDevices[DeviceManager::VideoInput].append(Device(device));
                emit q->devicesChanged(DeviceManager::VideoInput);
            }
        }
    }
}

void DeviceManagerPrivate::onDeviceAdded(const QString & udi)
{
    Solid::Device solidDevice(udi);
    addDevice(solidDevice);
}

void DeviceManagerPrivate::onDeviceRemoved(const QString & udi)
{
    //i here is DeviceManager::DeviceType. Loop done for: AudioInput, AudioOutput, VideoInput
    for (uint i=0; i<3; i++) {
        QList<Device>::iterator it = m_availableDevices[i].begin();
        for(; it != m_availableDevices[i].end(); ++it) {
            if ( (*it).udi() == udi ) {
                //we found the device that needs to be removed.
                //if the device that we are going to remove is the currently selected device,
                //we need to do some more work to select a new device...
                if ( (*it) == m_currentDevices[i] ) {
                    m_availableDevices[i].erase(it);
                    if ( m_availableDevices[i].size() > 0 ) {
                        m_currentDevices[i] = m_availableDevices[i][0];
                    } else {
                        m_currentDevices[i] = Device();
                    }
                } else {
                    //...else we just remove it
                    m_availableDevices[i].erase(it);
                }
                emit q->devicesChanged(static_cast<DeviceManager::DeviceType>(i));
                return;
            }
        }
    }
}

void DeviceManagerPrivate::addTestDevices()
{
    DevicePrivate d;
    d.name = i18n("Test input device");

    d.udi = QLatin1String("test_audio");
    d.driver = "audiotestsrc";
    m_availableDevices[DeviceManager::AudioInput].append(Device(d));

    d.udi = QLatin1String("test_video");
    d.driver = "videotestsrc";
    m_availableDevices[DeviceManager::VideoInput].append(Device(d));

    m_showTestDevices = true;
    emit q->devicesChanged(DeviceManager::AudioInput);
    emit q->devicesChanged(DeviceManager::VideoInput);
}

void DeviceManagerPrivate::removeTestDevices()
{
    onDeviceRemoved(QLatin1String("test_audio"));
    onDeviceRemoved(QLatin1String("test_video"));
}

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent), d(new DeviceManagerPrivate(this))
{
    d->detectDevices();
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)),
            SLOT(onDeviceAdded(QString)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)),
            SLOT(onDeviceRemoved(QString)));
}

DeviceManager::~DeviceManager()
{
    delete d;
}

bool DeviceManager::showsTestDevices() const
{
    return d->m_showTestDevices;
}

void DeviceManager::setShowTestDevices(bool show)
{
    if ( show && !d->m_showTestDevices ) {
        d->addTestDevices();
    } else if ( !show && d->m_showTestDevices ) {
        d->removeTestDevices();
    }
}

QList<Device> DeviceManager::availableDevices(DeviceType type) const
{
    return d->m_availableDevices[type];
}

Device DeviceManager::currentDeviceForType(DeviceType type) const
{
    return d->m_currentDevices[type];
}

void DeviceManager::setCurrentDeviceForType(DeviceType type, const Device & device)
{
    d->m_currentDevices[type] = device;
    emit currentDeviceChanged(type);
}

void DeviceManager::loadDefaults()
{
    for(int i=0; i<4; i++) { //i here is DeviceManager::DeviceType.
        if ( !d->m_availableDevices[i].isEmpty() ) {
            setCurrentDeviceForType(static_cast<DeviceType>(i), d->m_availableDevices[i][0]);
        }
    }
}

void DeviceManager::loadConfig(const KConfigGroup & config)
{
    QByteArray input = config.readEntry("PreferredDevices", QByteArray());
    QDataStream stream(&input, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_5);
    stream >> d->m_currentDevices[0] >> d->m_currentDevices[1]
           >> d->m_currentDevices[2] >> d->m_currentDevices[3];

    //here we do some sanity checks to verify that the devices that were read from the config
    //actually exist in the available devices list, and in addition if they exist we have to
    //get their description/udi from there, as these are not saved in the config.
    for(int i=0; i<4; i++) {
        bool found = false;
        foreach(const Device & availableDevice, d->m_availableDevices[i]) {
            if ( d->m_currentDevices[i] == availableDevice ) {
                setCurrentDeviceForType(static_cast<DeviceType>(i), availableDevice); //get description/udi
                found = true;
            }
        }

        //if the device read from the config is not in the list of available devices,
        //do not use it and use the first available device instead
        if ( !found && d->m_availableDevices[i].size() > 0 ) {
            setCurrentDeviceForType(static_cast<DeviceType>(i), d->m_availableDevices[i][0]);
        }
    }
}

void DeviceManager::saveConfig(KConfigGroup config) const
{
    QByteArray output;
    QDataStream stream(&output, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_5);
    stream << d->m_currentDevices[0] << d->m_currentDevices[1]
           << d->m_currentDevices[2] << d->m_currentDevices[3];
    config.writeEntry("PreferredDevices", output);
}

QtGstreamer::QGstElementPtr DeviceManager::newAudioInputElement()
{
    using namespace QtGstreamer;
    Device device = d->m_currentDevices[AudioInput];
    QGstElementPtr element = QGstElementFactory::make(device.driver());
    if ( element.isNull() ) {
        kWarning() << "Could not construct" << device.driver();
        return QGstElementPtr();
    }

    if ( device.driver() == "alsasrc" || device.driver() == "osssrc" ) {
        element->setProperty("device", device.driverHandle().toByteArray());
    }

    return element;
}

QtGstreamer::QGstElementPtr DeviceManager::newAudioOutputElement()
{
    using namespace QtGstreamer;
    Device device = d->m_currentDevices[AudioOutput];
    QGstElementPtr element = QGstElementFactory::make(device.driver());
    if ( element.isNull() ) {
        kWarning() << "Could not construct" << device.driver();
        return QGstElementPtr();
    }

    if ( device.driver() == "alsasink" || device.driver() == "osssink" ) {
        element->setProperty("device", device.driverHandle().toByteArray());
    }

    return element;
}

QtGstreamer::QGstElementPtr DeviceManager::newVideoInputElement()
{
    using namespace QtGstreamer;
    Device device = d->m_currentDevices[VideoInput];
    QGstElementPtr element = QGstElementFactory::make(device.driver());
    if ( element.isNull() ) {
        kWarning() << "Could not construct" << device.driver();
        return QGstElementPtr();
    }

    if ( device.driver() == "v4l2src" || device.driver() == "v4lsrc" ) {
        element->setProperty("device", device.driverHandle().toByteArray());
    }

//this part is for kdelibs < r1006441
#if !KDE_IS_VERSION(4, 3, 63)
# ifdef __GNUC__
#  warning "Using the v4l2 vs v4l1 detection hack"
# endif
    //HACK to detect if the device has a v4l1 or v4l2 driver
    if ( device.driver() == "v4lsrc" && !element->setState(QGstElement::Paused) ) {
        QGstElementPtr element_v4l2 = QGstElementFactory::make("v4l2src");
        if ( element_v4l2.isNull() ) {
            kWarning() << "Could not construct v4l2src";
            //fall back and return the v4lsrc, although it is not going to work...
            return element;
        }

        element_v4l2->setProperty("device", device.driverHandle().toByteArray());
        return element_v4l2;
    }
#endif

    return element;
}

VideoWidget *DeviceManager::newVideoWidget(QWidget *parent)
{
    using namespace QtGstreamer;
    Device device = d->m_currentDevices[VideoOutput];
    QGstElementPtr element = QGstElementFactory::make(device.driver());
    if ( element.isNull() ) {
        kWarning() << "Could not construct" << device.driver();
        return NULL;
    }

    AbstractRenderer *renderer = new OverlayRenderer(element);
    VideoWidget *widget = VideoWidget::newVideoWidget(renderer, parent);
    if ( !widget ) {
        delete renderer;
    }

    return widget;
}


//END DeviceManager

}

#include "devicemanager.moc"
