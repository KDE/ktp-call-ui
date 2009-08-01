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
#ifndef ACCOUNTITEM_H
#define ACCOUNTITEM_H

#include "treemodel.h"
#include <TelepathyQt4/Account>

class AccountItem : public QObject, public TreeModelItem
{
    Q_OBJECT
public:
    AccountItem(const Tp::AccountPtr & account, TreeModelItem *parent);

    virtual QVariant data(int role) const;

private slots:
    void onAccountReady(Tp::PendingOperation *op);
    void onAccountHaveConnectionChanged(bool);
    void onContactsReady(Tp::PendingOperation*);
    void onAccountInvalidated(Tp::DBusProxy *proxy, const QString & errorName,
                              const QString & errorMessage);
    //make emitDataChange available as a slot
    inline void emitDataChange() { TreeModelItem::emitDataChange(); }

private:
    QString iconForPresence(uint presenceType) const;
    Tp::AccountPtr m_account;
};

Q_DECLARE_METATYPE(Tp::AccountPtr);

#endif
