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

ContactItem::ContactItem(const Tp::ContactPtr & contact, TreeModelItem *parent, TreeModel *model)
    : ContactsModelItem(parent, model), m_contact(contact)
{
    connect(m_contact.data(), SIGNAL(aliasChanged(QString)), SLOT(emitDataChange()));
    connect(m_contact.data(), SIGNAL(simplePresenceChanged(QString, uint, QString)),
            SLOT(emitDataChange()));
}

QVariant ContactItem::data(int role) const
{
    switch(role) {
    case Qt::DisplayRole:
        return m_contact->alias();
    case Qt::DecorationRole:
        return iconForPresence((Tp::ConnectionPresenceType)m_contact->presenceType());
    case KCall::ItemTypeRole:
        return QByteArray("contact");
    case KCall::ObjectPtrRole:
        return QVariant::fromValue(m_contact);
    default:
        return QVariant();
    }
}
