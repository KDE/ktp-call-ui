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
#ifndef KGSTDEVICES_DEVICEMANAGER_H
#define KGSTDEVICES_DEVICEMANAGER_H

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

/** <p> This class' purpose is to manage audio/video input/output devices.
 * It can detect and list devices and also construct gstreamer elements
 * for the detected devices. </p>
 * <p> Devices are separated into four types, listed in the DeviceType enum.
 * For each type there is a list of available devices and a "current" device,
 * which is the device that will be used when constructing a gstreamer element
 * using one of the new*Element() functions and newVideoWidget(). </p>
 * <p>The lists of available devices for AudioInput, AudioOutput and VideoInput
 * also contain a pseudo-device called "Autodetect", which is always available
 * and is the default device for these types. When constructing an element for
 * this device, one of gstreamer's "auto{audio,video}{src,sink}" is returned,
 * depending on the type. </p>
 * <p> This class also offers a method to save and later restore the device
 * preferences, i.e. which devices were set for each type using
 * setCurrentDeviceForType(), which is useful for applications that want the
 * user to select device preferences using a separate GUI and later construct
 * the appropriate elements according to the user's preference. </p>
 */
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

    /** Returns a list with all the available devices for the given @a type. */
    QList<Device> availableDevices(DeviceType type) const;

    /** Returns the device that is currently selected as the preferred
     * one for the given @a type.
     */
    Device currentDeviceForType(DeviceType type) const;

    /** Sets the current @a device for the given @a type. This is used to set
     * a device preference. The element construction functions will always honor
     * this preference and create elements that represent this device, even if
     * this device is not working.
     */
    void setCurrentDeviceForType(DeviceType type, const Device & device);

    /** Loads the default devices for each type. For all types, except
     * VideoOutput, these are internal pseudo-devices that do autodetection
     * internally.
     */
    void loadDefaults();

    /** Loads device preferences from the "PreferredDevices" field of the given
     * @a config group. This is used to restore device preferences that were
     * saved using saveConfig(). If a device that was saved is not still
     * available on the system, it will fall back and use the default device for
     * this type.
     */
    void loadConfig(const KConfigGroup & config);

    /** Saves the currently selected devices (which were selected using
     * setCurrentDeviceForType()) in the "PreferredDevices" field of the
     * given @a config group. These can later be restored using loadConfig().
     * This is typically used to "remember" the user's device preferences.
     */
    void saveConfig(KConfigGroup config) const;

    /** Constructs a gstreamer element that represents an audio input device.
     * The element returned uses the device that has been set as current
     * (using setCurrentDeviceForType() for type == AudioInput) to do audio
     * input. If no device has been set as current, the gstreamer
     * "autoaudiosrc" element is returned.
     */
    QtGstreamer::QGstElementPtr newAudioInputElement();

    /** Constructs a gstreamer element that represents an audio output device.
     * The element returned uses the device that has been set as current
     * (using setCurrentDeviceForType() for type == AudioOutput) to do audio
     * output. If no device has been set as current, the gstreamer
     * "autoaudiosink" element is returned.
     */
    QtGstreamer::QGstElementPtr newAudioOutputElement();

    /** Constructs a gstreamer element that represents an video input device.
     * The element returned uses the device that has been set as current
     * (using setCurrentDeviceForType() for type == VideoInput) to capture
     * video. If no device has been set as current, the gstreamer
     * "autovideosrc" element is returned.
     */
    QtGstreamer::QGstElementPtr newVideoInputElement();

    /** Constructs a new VideoWidget with @a parent as a parent. The video
     * widget returned uses the video system that has been set as current
     * (using setCurrentDeviceForType() for type == VideoOutput) to display
     * video. Note that in case of video output, the physical device is always
     * the screen, so "device" in this case actually means the rendering system,
     * ex. X11, Xv, SDL, OpenGL, DirectShow, etc...
     * The default system is the software renderering system, that will convert
     * the frames into QImages and paint them with QPainter directly on the
     * VideoWidget.
     * @todo Implement the software renderer. Currently Xv is the default.
     * @todo Make this function usable for non-X11 platforms.
     * Currently only Xv and X11 are available as choices here.
     */
    VideoWidget *newVideoWidget(QWidget *parent = 0);

Q_SIGNALS:
    /** This signal is emmited when the list of available devices has changed
     * for the given @a type. This can happen if the user plugs in/out some
     * usb camera, for example.
     */
    void devicesChanged(DeviceManager::DeviceType type);

    /** This signal is emmited when the current device for the given @a type
     * changes. This happens when somebody alters the current device using one
     * of the available methods of this class.
     */
    void currentDeviceChanged(DeviceManager::DeviceType type);

private:
    friend struct DeviceManagerPrivate;
    DeviceManagerPrivate *const d;
    Q_PRIVATE_SLOT(d, void onDeviceAdded(const QString & udi))
    Q_PRIVATE_SLOT(d, void onDeviceRemoved(const QString & udi))
};

/** This class represents a device. A device here is an abstract entity that
 * can be anything from a physical device to a video API like SDL and DirectShow.
 * This is meant to be used with DeviceManager only.
 */
class KGSTDEVICES_EXPORT Device
{
public:
    Device(const Device & other);
    virtual ~Device();

    Device & operator=(const Device & other);
    bool operator==(const Device & other) const;
    bool isValid() const;

    /** Returns a human-readable (possibly translated) string
     * that describes the device.
     */
    QString name() const;

    /** Returns a string that mentions which "driver" should be used to handle
     * this device. This is usually the name of the gstreamer element that
     * should be constructed to use this device.
     */
    QByteArray driver() const;

    /** This describes some properties that should be given to the "driver"
     * in order to choose and use this device. The variant has driver-dependent
     * contents that only DeviceManager knows how to interpret properly.
     */
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
