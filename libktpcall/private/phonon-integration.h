/*
    This file contains code from the phonon project.

    Copyright (C) 2006-2008 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Nokia Corporation
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PHONON_INTEGRATION_P_H
#define PHONON_INTEGRATION_P_H

#include <phonon/Global>
#include <phonon/ObjectDescription>

class PhononIntegration
{
public:
    static QList<Phonon::DeviceAccessList> readDevices(Phonon::ObjectDescriptionType type,
                                                       Phonon::Category category);

private:
    static QList<Phonon::DeviceAccessList> readAudioDevices(Phonon::ObjectDescriptionType type,
                                                            Phonon::Category category);
    static QList<Phonon::DeviceAccessList> readVideoDevices(Phonon::ObjectDescriptionType type,
                                                            Phonon::Category category);
    static bool hideAdvancedDevices();
    static QList<int> sortDevicesByCategoryPriority(Phonon::ObjectDescriptionType type,
                                                    Phonon::Category category,
                                                    QList<int> originalList);
};

#endif // PHONONINTEGRATION_P_H
