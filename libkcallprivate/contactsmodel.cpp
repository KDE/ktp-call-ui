/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "contactsmodel.h"
#include "accountitem.h"
#include <KDebug>

ContactsModelItem::ContactsModelItem(TreeModelItem *parent, TreeModel *model)
    : QObject(), TreeModelItem(parent, model)
{
}

KIcon ContactsModelItem::iconForPresence(Tp::ConnectionPresenceType presenceType) const
{
    switch (presenceType) {
    case Tp::ConnectionPresenceTypeOffline:
        return KIcon("user-offline");
    case Tp::ConnectionPresenceTypeAvailable:
        return KIcon("user-online");
    case Tp::ConnectionPresenceTypeAway:
        return KIcon("user-away");
    case Tp::ConnectionPresenceTypeExtendedAway:
        return KIcon("user-away-extended");
    case Tp::ConnectionPresenceTypeHidden:
        return KIcon("user-invisible");
    case Tp::ConnectionPresenceTypeBusy:
        return KIcon("user-busy");
    default:
        kWarning() << "presence type is unset/unknown/invalid. value:" << presenceType;
        return KIcon();
    }
}


ContactsModel::ContactsModel(QObject *parent)
    : TreeModel(parent)
{
}

void ContactsModel::addAccount(const QString & busName, const QString & path)
{
    kDebug() << "Adding account" << busName << path;

    AccountItem *item = new AccountItem(busName, path, root(), this);
    root()->appendChild(item);
}

#include "contactsmodel.moc"
