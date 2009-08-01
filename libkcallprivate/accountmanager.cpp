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
#include "accountmanager.h"
#include "accountitem.h"
#include <KDebug>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/PendingReady>

class AccountsContactsTreeModel : public TreeModel
{
public:
    inline AccountsContactsTreeModel(QObject *parent = 0) : TreeModel(parent) {}

    inline void addAccount(const Tp::AccountPtr & account)
    {
        AccountItem *item = new AccountItem(account, root());
        root()->appendChild(item);
    }
};

struct AccountManager::Private
{
    Private(AccountManager *qq) : q(qq) {}

    void init();
    void onAccountManagerReady(Tp::PendingOperation *op);
    void onAccountCreated(const QString & path);

    Tp::AccountManagerPtr m_accountManager;
    AccountsContactsTreeModel *m_accountsContactsTreeModel;
    AccountManager *const q;
};

void AccountManager::Private::init()
{
    m_accountManager = Tp::AccountManager::create();
    connect(m_accountManager->becomeReady(), SIGNAL(finished(Tp::PendingOperation*)),
            q, SLOT(onAccountManagerReady(Tp::PendingOperation*)));

    m_accountsContactsTreeModel = new AccountsContactsTreeModel(q);
}

void AccountManager::Private::onAccountManagerReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "Account manager failed to become ready:" << op->errorMessage();
        return; //TODO handle this error
    }

    foreach(const QString & path, m_accountManager->validAccountPaths()) {
        Tp::AccountPtr account = m_accountManager->accountForPath(path);
        Q_ASSERT(!account.isNull());
        m_accountsContactsTreeModel->addAccount(account);
    }

    connect(m_accountManager.data(), SIGNAL(accountCreated(QString)),
            q, SLOT(onAccountCreated(QString)));
}

void AccountManager::Private::onAccountCreated(const QString & path)
{
    Tp::AccountPtr account = m_accountManager->accountForPath(path);
    Q_ASSERT(!account.isNull());
    m_accountsContactsTreeModel->addAccount(account);
}

AccountManager::AccountManager(QObject *parent)
    : QObject(parent), d(new Private(this))
{
    d->init();
}

AccountManager::~AccountManager()
{
    delete d;
}

QAbstractItemModel *AccountManager::accountsModel() const
{
    return d->m_accountsContactsTreeModel;
}

QAbstractItemModel *AccountManager::contactsModel() const
{
    return d->m_accountsContactsTreeModel;
}

#include "accountmanager.moc"
