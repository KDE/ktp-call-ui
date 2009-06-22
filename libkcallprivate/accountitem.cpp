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
#include "accountitem.h"
#include "pendingcontacts.h"
#include "contactitem.h"
#include <KLocalizedString>
#include <KDebug>
#include <TelepathyQt4/PendingReady>

AccountItem::AccountItem(const QString & busName, const QString & path,
                         TreeModelItem *parent, TreeModel *model)
    : ContactsModelItem(parent, model)
{
    m_account = Tp::Account::create(busName, path);
    connect(m_account->becomeReady(), SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountReady(Tp::PendingOperation*)));
}

QVariant AccountItem::data(int role) const
{
    if ( !m_account->isReady() ) {
        if ( role == Qt::DisplayRole ) {
            return i18nc("@info:status", "Waiting for account information...");
        } else {
            return QVariant();
        }
    } else {
        switch(role) {
        case Qt::DisplayRole:
            return m_account->displayName();
        case Qt::DecorationRole:
            return iconForPresence((Tp::ConnectionPresenceType)m_account->currentPresence().type);
        case ContactsModel::ItemTypeRole:
            return QByteArray("account");
        case ContactsModel::ObjectPtrRole:
            return QVariant::fromValue(m_account);
        default:
            return QVariant();
        }
    }
}

void AccountItem::onAccountReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "Account failed to become ready" << op->errorName() << op->errorMessage();
        return; //TODO and then what?
    }

    connect(m_account.data(), SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            SLOT(onAccountInvalidated(Tp::DBusProxy*, QString, QString)));
    connect(m_account.data(), SIGNAL(haveConnectionChanged(bool)),
            SLOT(onAccountHaveConnectionChanged(bool)) );

    connect(m_account.data(), SIGNAL(stateChanged(bool)), SLOT(emitDataChange()));
    connect(m_account.data(), SIGNAL(displayNameChanged(const QString &)), SLOT(emitDataChange()));
    connect(m_account.data(), SIGNAL(nicknameChanged(const QString &)), SLOT(emitDataChange()));
    connect(m_account.data(), SIGNAL(currentPresenceChanged(const Tp::SimplePresence &)),
            SLOT(emitDataChange()));
    connect(m_account.data(), SIGNAL(haveConnectionChanged(bool)), SLOT(emitDataChange()));
/*
    connect(m_account.data(),
            SIGNAL(connectionStatusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)),
            SLOT(onConnectionStatusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)));
*/

    if ( m_account->haveConnection() ) {
        connect(new PendingContacts(m_account, this),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContactsReady(Tp::PendingOperation*)) );
    }

    emitDataChange();
}

void AccountItem::onAccountInvalidated(Tp::DBusProxy *proxy, const QString & errorName,
                                       const QString & errorMessage)
{
    Q_UNUSED(proxy);
    kDebug() << "Account" << m_account->displayName() << "was invalidated:"
             << errorName << errorMessage;

    if ( childrenCount() > 0 ) {
        removeChildren(0, childrenCount()-1);
    }
    parentItem()->removeChild(parentItem()->rowOfChild(this)); //NOTE 'this' is deleted here
}

void AccountItem::onAccountHaveConnectionChanged(bool hasConnection)
{
    if ( !hasConnection ) {
        if ( childrenCount() > 0 ) {
            removeChildren(0, childrenCount()-1);
        }
    } else {
        connect(new PendingContacts(m_account, this),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContactsReady(Tp::PendingOperation*)) );
    }
}

void AccountItem::onContactsReady(Tp::PendingOperation *op)
{
    if (op->isError()) {
        kError() << "Failed to get contacts list" << op->errorName() << op->errorMessage();
        return;
    }

    PendingContacts *pc = qobject_cast<PendingContacts*>(op);
    Q_ASSERT(pc);

    QVector<TreeModelItem*> contactItems;
    foreach(const Tp::ContactPtr & contact, pc->contacts()) {
        contactItems.append(new ContactItem(contact, this, model()));
    }
    appendChildren(contactItems);
}

#include "accountitem.moc"
