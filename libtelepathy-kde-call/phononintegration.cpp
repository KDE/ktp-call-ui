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
#include "phononintegration_p.h"
#include <QtCore/QSettings>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <KDebug>
#include <KGlobal>

Q_DECLARE_METATYPE(QList<int>)

struct PhononIntegrationPrivate
{
    PhononIntegrationPrivate()
        : phononSettings(QLatin1String("kde.org"), QLatin1String("libphonon")),
          phononServer(QLatin1String("org.kde.kded"),
                       QLatin1String("/modules/phononserver"),
                       QLatin1String("org.kde.PhononServer"))
    {
        registerMetaTypes();
    }

    void registerMetaTypes()
    {
        // Register the PhononDeviceAccessList type if it has not been registered yet
        if (qMetaTypeId<Phonon::DeviceAccess>())
        {
            qRegisterMetaType<Phonon::DeviceAccess>();
            qRegisterMetaTypeStreamOperators<Phonon::DeviceAccess>("Phonon::DeviceAccess");
        }

        if (qMetaTypeId<Phonon::DeviceAccessList>())
        {
            qRegisterMetaType<Phonon::DeviceAccessList>();
            qRegisterMetaTypeStreamOperators<Phonon::DeviceAccessList>("Phonon::DeviceAccessList");

            //compatibility with kde 4.5
            qRegisterMetaTypeStreamOperators<Phonon::DeviceAccessList>("PhononDeviceAccessList");
        }
    }

    QSettings phononSettings;
    QDBusInterface phononServer;
};

K_GLOBAL_STATIC(PhononIntegrationPrivate, s_priv);


QList<Phonon::DeviceAccessList> PhononIntegration::readDevices(Phonon::ObjectDescriptionType type,
                                                               Phonon::Category category)
{
    switch (type) {
    case Phonon::AudioOutputDeviceType:
    case Phonon::AudioCaptureDeviceType:
        return readAudioDevices(type, category);
        break;
    case Phonon::VideoCaptureDeviceType:
        return readVideoDevices(category);
        break;
    default:
        return QList<Phonon::DeviceAccessList>();
    }
}

template <typename R>
static inline R dbusCall(QDBusInterface & interface, const QString & method, int argument)
{
    R r;
    QDBusReply<QByteArray> reply = interface.call(method, argument);
    if (!reply.isValid()) {
        kDebug() << reply.error();
        return r;
    }

    QDataStream stream(reply.value());
    stream >> r;
    return r;
}

QList<Phonon::DeviceAccessList> PhononIntegration::readAudioDevices(Phonon::ObjectDescriptionType type,
                                                                    Phonon::Category category)
{
    QList<Phonon::DeviceAccessList> list;

    QList<int> indices = dbusCall< QList<int> >(s_priv->phononServer,
                                                QLatin1String("audioDevicesIndexes"), type);
    kDebug() << "got audio device indices" << indices << "for type" << type;

    indices = sortDevicesByCategoryPriority(type, category, indices);
    kDebug() << "sorted audio devices indices" << indices << "for category" << category;

    for (int i=0; i < indices.size(); ++i) {
        QHash<QByteArray, QVariant> properties =
            dbusCall< QHash<QByteArray, QVariant> >(s_priv->phononServer,
                                                    QLatin1String("audioDevicesProperties"),
                                                    indices.at(i));

        if (hideAdvancedDevices()) {
            const QVariant var = properties.value("isAdvanced");
            if (var.isValid() && var.toBool()) {
                kDebug() << "hiding device" << indices.at(i)
                         << "because it is advanced and HideAdvancedDevices is specified";
                continue;
            }
        }

        QVariant accessList = properties.value("deviceAccessList");
        if (accessList.isValid()) {
            kDebug() << "appending device access list for device" << indices.at(i);
            list.append(accessList.value<Phonon::DeviceAccessList>());
        }
    }
    return list;
}

QList<Phonon::DeviceAccessList> PhononIntegration::readVideoDevices(Phonon::Category category)
{
    return QList<Phonon::DeviceAccessList>(); //TODO implement me
}

bool PhononIntegration::hideAdvancedDevices()
{
    return s_priv->phononSettings.value(QLatin1String("General/HideAdvancedDevices"), true).toBool();
}

QList<int> PhononIntegration::sortDevicesByCategoryPriority(Phonon::ObjectDescriptionType type,
                                                            Phonon::Category category,
                                                            QList<int> originalList)
{
    switch(type) {
    case Phonon::AudioOutputDeviceType:
        s_priv->phononSettings.beginGroup(QLatin1String("AudioOutputDevice"));
        break;
    case Phonon::AudioCaptureDeviceType:
        s_priv->phononSettings.beginGroup(QLatin1String("AudioCaptureDevice"));
        break;
    case Phonon::VideoCaptureDeviceType:
        s_priv->phononSettings.beginGroup(QLatin1String("VideoCaptureDevice"));
        break;
    default:
        Q_ASSERT(false);
    }

    if (originalList.size() <= 1) {
        // nothing to sort
        return originalList;
    } else {
        // make entries unique
        QSet<int> seen;
        QMutableListIterator<int> it(originalList);
        while (it.hasNext()) {
            if (seen.contains(it.next())) {
                it.remove();
            } else {
                seen.insert(it.value());
            }
        }
    }

    QString categoryKey = QLatin1String("Category_") + QString::number(static_cast<int>(category));
    if (!s_priv->phononSettings.contains(categoryKey)) {
        // no list in config for the given category
        categoryKey = QLatin1String("Category_") + QString::number(static_cast<int>(Phonon::NoCategory));
        if (!s_priv->phononSettings.contains(categoryKey)) {
            // no list in config for NoCategory
            return originalList;
        }
    }

    //Now the list from the config
    QList<int> deviceList = s_priv->phononSettings.value(categoryKey).value< QList<int> >();

    //if there are devices in the config that phononserver doesn't report, remove them from the list
    QMutableListIterator<int> i(deviceList);
    while (i.hasNext()) {
        if (0 == originalList.removeAll(i.next())) {
            i.remove();
        }
    }

    //if the backend reports more devices that are not in d->config append them to the list
    deviceList += originalList;

    s_priv->phononSettings.endGroup();
    return deviceList;
}
