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
#ifndef _KGSTDEVICES_MEDIADEVICES_H
#define _KGSTDEVICES_MEDIADEVICES_H

#include "volumecontrolinterface.h"
#include "devicefactory.h"
typedef struct _GstElement GstElement;

namespace KGstDevices {

class KGSTDEVICES_EXPORT Device
{
    Q_DISABLE_COPY(Device)
public:
    virtual ~Device();
    GstElement *bin() const;
    GstElement *mainElement() const;

protected:
    Device(GstElement *bin, GstElement *mainElement);

private:
    struct Private;
    Private *const d;
};

class KGSTDEVICES_EXPORT AudioDevice : public Device, public VolumeControlInterface
{
    Q_DISABLE_COPY(AudioDevice)
public:
    static AudioDevice *createAudioInputDevice(GstElement *element);
    static AudioDevice *createAudioOutputDevice(GstElement *element);

    virtual ~AudioDevice();

    virtual bool volumeControlIsAvailable() const;

    virtual bool isMuted() const;
    virtual void setMuted(bool mute);

    virtual int minVolume() const;
    virtual int maxVolume() const;
    virtual int volume() const;
    virtual void setVolume(int volume);

protected:
    AudioDevice(GstElement *bin, GstElement *mainElement, GstElement *volumeElement);

private:
    struct Private;
    Private *const d;
};

}

#endif
