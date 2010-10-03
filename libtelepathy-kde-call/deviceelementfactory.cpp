/*
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

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
#include "deviceelementfactory_p.h"
#include "phononintegration_p.h"
#include <QGst/Bin>
#include <QGst/ElementFactory>
#include <KDebug>
#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>

QGst::ElementPtr DeviceElementFactory::makeAudioCaptureElement()
{
    QGst::ElementPtr element;

    //allow overrides from the application's configuration file
    element = tryOverrideForKey("audiosrc");
    if (element) {
        return element;
    }

    //use gconf on non-kde environments
    if (qgetenv("KDE_FULL_SESSION").isEmpty()) {
        element = tryElement("gconfaudiosrc");
        return element;
    }

    //for kde environments try to do what phonon does:
    //first try pulseaudio,
    element = tryElement("pulsesrc");
    if (element) {
        return element;
    }

    //then try alsa/oss devices reported by phononserver
    //in the order that they are configured in phonon
    QList<Phonon::DeviceAccessList> phononDeviceLists
        = PhononIntegration::readDevices(Phonon::AudioCaptureDeviceType, Phonon::CommunicationCategory);

    Q_FOREACH(const Phonon::DeviceAccessList & deviceList, phononDeviceLists) {
        Q_FOREACH(const Phonon::DeviceAccess & device, deviceList) {
            if (device.first == "alsa") {
                //skip x-phonon devices, we will use plughw which is always second in the list
                if (!device.second.startsWith("x-phonon")) {
                    element = tryElement("alsasrc", device.second);
                }
            } else if (device.first == "oss") {
                element = tryElement("osssrc", device.second);
            }

            if (element) {
                return element;
            }
        }
    }

    //as a last resort, try gstreamer's autodetection
    element = tryElement("autoaudiosrc");
    return element;
}

QGst::ElementPtr DeviceElementFactory::makeAudioOutputElement()
{
    QGst::ElementPtr element;

    //allow overrides from the application's configuration file
    element = tryOverrideForKey("audiosink");
    if (element) {
        return element;
    }

    //use gconf on non-kde environments
    if (qgetenv("KDE_FULL_SESSION").isEmpty()) {
        element = tryElement("gconfaudiosink");
        if (element) {
            element->setProperty("profile", 2 /*chat*/);
        }
        return element;
    }

    //for kde environments try to do what phonon does:
    //first try pulseaudio,
    element = tryElement("pulsesink");
    if (element) {
        //TODO set category somehow...
        return element;
    }

    //then try alsa/oss devices reported by phononserver
    //in the order that they are configured in phonon
    QList<Phonon::DeviceAccessList> phononDeviceLists
        = PhononIntegration::readDevices(Phonon::AudioOutputDeviceType, Phonon::CommunicationCategory);

    Q_FOREACH(const Phonon::DeviceAccessList & deviceList, phononDeviceLists) {
        Q_FOREACH(const Phonon::DeviceAccess & device, deviceList) {
            if (device.first == "alsa") {
                //use dmix instead of x-phonon, since we don't have phonon's alsa configuration file
                QString deviceString = device.second;
                deviceString.replace("x-phonon", "dmix");
                element = tryElement("alsasink", deviceString);
            } else if (device.first == "oss") {
                element = tryElement("osssink", device.second);
            }

            if (element) {
                return element;
            }
        }
    }

    //as a last resort, try gstreamer's autodetection
    element = tryElement("autoaudiosink");
    return element;
}

QGst::ElementPtr DeviceElementFactory::makeVideoCaptureElement()
{
    QGst::ElementPtr element;

    //allow overrides from the application's configuration file
    element = tryOverrideForKey("videosrc");
    if (element) {
        return element;
    }

    //use gconf on non-kde environments
    if (qgetenv("KDE_FULL_SESSION").isEmpty()) {
        element = tryElement("gconfvideosrc");
        return element;
    }

    //TODO implement phonon settings

    //as a last resort, try gstreamer's autodetection
    element = tryElement("autovideosrc");
    return element;
}

QGst::ElementPtr DeviceElementFactory::tryElement(const char *name, const QString & device)
{
    QGst::ElementPtr element = QGst::ElementFactory::make(name);
    if (!element) {
        kDebug() << "Could not make element" << name;
        return element;
    }

    if (!device.isEmpty()) {
        try {
            element->setProperty("device", device);
        } catch (const std::logic_error & error) {
            kDebug() << "FIXME: Element" << name << "doesn't support string device property";
        }
    }

    if (!element->setState(QGst::StateReady)) {
        kDebug() << "Element" << name << "with device string" << device << "doesn't want to become ready";
        return QGst::ElementPtr();
    }

    kDebug() << "Using element" << name << "with device string" << device;
    return element;
}

QGst::ElementPtr DeviceElementFactory::tryOverrideForKey(const char *keyName)
{
    QGst::ElementPtr element;
    const KConfigGroup configGroup = KGlobal::config()->group("GStreamer");

    if (configGroup.hasKey(keyName)) {
        QString binDescription = configGroup.readEntry(keyName);
        element = QGst::Bin::fromDescription(binDescription.toUtf8());

        if (!element) {
            kDebug() << "Could not construct bin" << binDescription;
            return element;
        }

        if (!element->setState(QGst::StateReady)) {
            kDebug() << "Custom bin" << binDescription << "doesn't want to become ready";
            return QGst::ElementPtr();
        }

        kDebug() << "Using custom bin" << binDescription;
    }

    return element;
}
