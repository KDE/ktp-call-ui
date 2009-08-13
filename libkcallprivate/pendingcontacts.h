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
#ifndef PENDINGCONTACTS_H
#define PENDINGCONTACTS_H

#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Account>
#include <TelepathyQt4/Contact>

class PendingContacts : public Tp::PendingOperation
{
    Q_OBJECT
public:
    PendingContacts(Tp::AccountPtr account, QObject *parent);

    Tp::AccountPtr account() const;
    Tp::Contacts contacts() const;

private slots:
    void onConnectionReady(Tp::PendingOperation *op);
    void onContactsReady(Tp::PendingOperation *op);

private:
    Tp::AccountPtr m_account;
    Tp::Contacts m_contacts;
};

#endif
