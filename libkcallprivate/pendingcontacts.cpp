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
#include "pendingcontacts.h"
#include <KDebug>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ContactManager>

PendingContacts::PendingContacts(Tp::AccountPtr account, QObject *parent)
    : Tp::PendingOperation(parent), m_account(account)
{
    Q_ASSERT(account->haveConnection());

    Tp::ConnectionPtr connection = account->connection();
    connect(connection->becomeReady(Tp::Connection::FeatureRoster),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onConnectionReady(Tp::PendingOperation*)));
}

Tp::AccountPtr PendingContacts::account() const
{
    return m_account;
}

Tp::Contacts PendingContacts::contacts() const
{
    return m_contacts;
}

void PendingContacts::onConnectionReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
        Q_ASSERT(pr);
        Tp::Connection *connection = qobject_cast<Tp::Connection*>(pr->object());
        Q_ASSERT(connection);

        m_contacts = connection->contactManager()->allKnownContacts();
        kDebug() << "received" << m_contacts.size() << "contacts";
        setFinished();
    }
}

#include "pendingcontacts.moc"
