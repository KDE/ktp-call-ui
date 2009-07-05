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
#ifndef _KGSTDEVICES_DEVICEFACTORY_H
#define _KGSTDEVICES_DEVICEFACTORY_H

#include "kgstdevices_export.h"
#include <QtCore/QString>
#include <KConfigGroup>
template <class T> class QSharedPointer;
typedef struct _GstElement GstElement;

namespace KGstDevices {

class AudioDevice;
struct ElementDescription;
typedef QSharedPointer<const ElementDescription> ElementDescriptionPtr;

class KGSTDEVICES_EXPORT DeviceFactory
{
public:
    DeviceFactory(const KConfigGroup & group = KConfigGroup());
    virtual ~DeviceFactory();

    bool createAudioInputDevice();
    bool createAudioOutputDevice();

    AudioDevice *audioInputDevice() const;
    AudioDevice *audioOutputDevice() const;

    bool hasError() const;
    QStringList errorLog() const;
    void clearLog();

private:
    bool canOpenAudioDevice(GstElement *element) const;
    void loadDeviceSettings(GstElement *element, ElementDescriptionPtr description) const;
    GstElement *createAudioSource(ElementDescriptionPtr description);
    GstElement *createAudioSink(ElementDescriptionPtr description);

    struct Private;
    Private *const d;
};

}

#endif
