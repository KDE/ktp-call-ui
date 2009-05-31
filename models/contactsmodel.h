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
#ifndef CONTACTSMODEL_H
#define CONTACTSMODEL_H

#include "treemodel.h"
#include <KIcon>
#include <TelepathyQt4/Constants>

class ContactsModelItem : public QObject, public TreeModelItem
{
    Q_OBJECT
public:
    ContactsModelItem(TreeModelItem *parent, TreeModel *model);

protected:
    KIcon iconForPresence(Tp::ConnectionPresenceType presenceType) const;

protected slots:
    //make emitDataChange available as a slot
    inline void emitDataChange() { TreeModelItem::emitDataChange(); }
};

class ContactsModel : public TreeModel
{
    Q_OBJECT
public:
    enum Role { ItemTypeRole = Qt::UserRole, ObjectPtrRole, };

    explicit ContactsModel(QObject *parent = 0);

public slots:
    void addAccount(const QString & busName, const QString & path);
};

#endif
