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
#include "contactitem.h"
#include "constants.h"
#include <KIcon>
#include <KDebug>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Presence>

ContactItem::ContactItem(const Tp::ContactPtr & contact, TreeModelItem *parent)
    : QObject(), TreeModelItem(parent), m_contact(contact)
{
    connect(m_contact.data(), SIGNAL(aliasChanged(QString)), SLOT(emitDataChange()));
    connect(m_contact.data(), SIGNAL(presenceChanged(Tp::Presence)),
            SLOT(emitDataChange()));
}

QVariant ContactItem::data(int role) const
{
    switch(role) {
    case Qt::DisplayRole:
        return m_contact->alias();
    case Qt::DecorationRole:
        return KIcon(iconForPresence(m_contact->presence().type()));
    case KCall::ItemTypeRole:
        return QByteArray("contact");
    case KCall::ObjectPtrRole:
        return QVariant::fromValue(m_contact);
    default:
        return QVariant();
    }
}

QString ContactItem::iconForPresence(uint presenceType) const
{
    switch (presenceType) {
    case Tp::ConnectionPresenceTypeOffline:
        return QLatin1String("im-user-offline");
    case Tp::ConnectionPresenceTypeAvailable:
        return QLatin1String("im-user");
    case Tp::ConnectionPresenceTypeAway:
    case Tp::ConnectionPresenceTypeExtendedAway:
        return QLatin1String("im-user-away");
    case Tp::ConnectionPresenceTypeHidden:
        return QLatin1String("im-invisible-user");
    case Tp::ConnectionPresenceTypeBusy:
        return QLatin1String("im-user-busy");
    default:
        kWarning() << "presence type is unset/unknown/invalid. value:" << presenceType;
        return QString();
    }
}

#include "contactitem.moc"
