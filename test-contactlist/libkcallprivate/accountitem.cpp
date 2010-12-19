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
#include "accountitem.h"
#include "pendingcontacts.h"
#include "contactitem.h"
#include "constants.h"
#include <KLocalizedString>
#include <KIcon>
#include <KDebug>
#include <TelepathyQt4/PendingReady>

AccountItem::AccountItem(const Tp::AccountPtr & account, TreeModelItem *parent)
    : QObject(), TreeModelItem(parent)
{
    m_account = account;
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
            if ( m_account->connectionStatus() == Tp::ConnectionStatusConnecting ) {
                return i18nc("protocol - display name (Connecting)", "%1 - %2 (Connecting)",
                             m_account->protocol(), m_account->displayName());
            } else {
                return i18nc("protocol - display name", "%1 - %2",
                             m_account->protocol(), m_account->displayName());
            }
        case Qt::DecorationRole:
        {
            uint presenceType = data(KCall::PresenceRole).value<Tp::SimplePresence>().type;
            return KIcon(iconForPresence(presenceType));
        }
        case KCall::PresenceRole:
        {
            Tp::SimplePresence presence = m_account->currentPresence();
            if ( presence.type == Tp::ConnectionPresenceTypeUnset ) {
                switch(m_account->connectionStatus()) {
                    case Tp::ConnectionStatusConnected:
                        presence.type = Tp::ConnectionPresenceTypeAvailable;
                        presence.status = "available";
                        break;
                    default:
                        presence.type = Tp::ConnectionPresenceTypeOffline;
                        presence.status = "offline";
                        break;
                }
            }
            return QVariant::fromValue(presence);
        }
        case KCall::ItemTypeRole:
            return QByteArray("account");
        case KCall::ObjectPtrRole:
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

    connect(m_account.data(), SIGNAL(haveConnectionChanged(bool)),
            SLOT(onAccountHaveConnectionChanged(bool)) );
    connect(m_account.data(), SIGNAL(stateChanged(bool)), SLOT(emitDataChange()));
    connect(m_account.data(), SIGNAL(displayNameChanged(const QString &)), SLOT(emitDataChange()));
    connect(m_account.data(), SIGNAL(nicknameChanged(const QString &)), SLOT(emitDataChange()));
    connect(m_account.data(), SIGNAL(currentPresenceChanged(const Tp::SimplePresence &)),
            SLOT(emitDataChange()));
    connect(m_account.data(), SIGNAL(haveConnectionChanged(bool)), SLOT(emitDataChange()));
    connect(m_account.data(),
            SIGNAL(connectionStatusChanged(Tp::ConnectionStatus, Tp::ConnectionStatusReason)),
            SLOT(emitDataChange()));

    if ( m_account->haveConnection() ) {
        connect(new PendingContacts(m_account, this),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onContactsReady(Tp::PendingOperation*)) );
    }

    emitDataChange();
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
        contactItems.append(new ContactItem(contact, this));
    }
    appendChildren(contactItems);
}

QString AccountItem::iconForPresence(uint presenceType) const
{
    switch (presenceType) {
    case Tp::ConnectionPresenceTypeOffline:
        return QLatin1String("user-offline");
    case Tp::ConnectionPresenceTypeAvailable:
        return QLatin1String("user-online");
    case Tp::ConnectionPresenceTypeAway:
        return QLatin1String("user-away");
    case Tp::ConnectionPresenceTypeExtendedAway:
        return QLatin1String("user-away-extended");
    case Tp::ConnectionPresenceTypeHidden:
        return QLatin1String("user-invisible");
    case Tp::ConnectionPresenceTypeBusy:
        return QLatin1String("user-busy");
    default:
        kWarning() << "presence type is unset/unknown/invalid. value:" << presenceType;
        return QString();
    }
}

#include "accountitem.moc"
