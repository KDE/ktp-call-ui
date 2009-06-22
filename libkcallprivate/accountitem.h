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

#include "contactsmodel.h"
#include <TelepathyQt4/Account>

class AccountItem : public ContactsModelItem
{
    Q_OBJECT
public:
    AccountItem(const QString & busName, const QString & path,
                TreeModelItem *parent, TreeModel *model);

    virtual QVariant data(int role) const;

private slots:
    void onAccountReady(Tp::PendingOperation *op);
    void onAccountHaveConnectionChanged(bool);
    void onContactsReady(Tp::PendingOperation*);
    void onAccountInvalidated(Tp::DBusProxy *proxy, const QString & errorName,
                              const QString & errorMessage);

private:
    Tp::AccountPtr m_account;
};

Q_DECLARE_METATYPE(Tp::AccountPtr);

#endif
