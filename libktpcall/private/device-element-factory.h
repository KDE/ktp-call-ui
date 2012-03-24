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
#ifndef DEVICE_ELEMENT_FACTORY_P_H
#define DEVICE_ELEMENT_FACTORY_P_H

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QGst/Global>

class DeviceElementFactory
{
public:
    static QGst::ElementPtr makeAudioCaptureElement();
    static QGst::ElementPtr makeAudioOutputElement();
    static QGst::ElementPtr makeVideoCaptureElement();

private:
    static QGst::ElementPtr tryElement(const char *name, const QString & device = QString());
    static QGst::ElementPtr tryOverrideForKey(const char *keyName);
};

#endif // DEVICE_ELEMENT_FACTORY_H
