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
#include "kcallapplication.h"
#include "mainwindow.h"
#include "callhandler.h"
#include "systrayicon.h"
#include "knotifyapprover.h"
#include "models/contactsmodel.h"
#include <KDebug>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ClientRegistrar>

struct KCallApplication::Private
{
    QPointer<MainWindow> mainWindow;
    ContactsModel *contactsModel;
    Tp::AccountManagerPtr accountManager;
    Tp::ClientRegistrarPtr registrar;
    Tp::SharedPtr<CallHandler> callHandler;
    Tp::SharedPtr<SystrayIcon> systrayIcon;
    Tp::SharedPtr<KNotifyApprover> knotifyApprover;
};

KCallApplication::KCallApplication()
    : KUniqueApplication(), d(new Private)
{
    d->mainWindow = NULL;
    d->contactsModel = new ContactsModel(this);
    d->accountManager = Tp::AccountManager::create();
    connect(d->accountManager->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onAccountManagerReady(Tp::PendingOperation *)));

    d->registrar = Tp::ClientRegistrar::create();
    d->callHandler = Tp::SharedPtr<CallHandler>(new CallHandler());
    d->registrar->registerClient(Tp::AbstractClientPtr::dynamicCast(d->callHandler), "kcall");
    d->systrayIcon = Tp::SharedPtr<SystrayIcon>(new SystrayIcon());
    d->registrar->registerClient(Tp::AbstractClientPtr::dynamicCast(d->systrayIcon), "kcall_approver_systray");
    d->knotifyApprover = Tp::SharedPtr<KNotifyApprover>(new KNotifyApprover());
    d->registrar->registerClient(Tp::AbstractClientPtr::dynamicCast(d->knotifyApprover), "kcall_approver_knotify");
}

KCallApplication::~KCallApplication()
{
    delete d;
}

int KCallApplication::newInstance()
{
    if ( !d->mainWindow ) {
        d->mainWindow = new MainWindow;
        d->mainWindow->show();
    }
    return 0;
}

ContactsModel *KCallApplication::contactsModel() const
{
    return d->contactsModel;
}

void KCallApplication::showHideMainWindow()
{
    if ( !d->mainWindow ) {
        d->mainWindow = new MainWindow;
        d->mainWindow->show();
    } else {
        d->mainWindow->setVisible( !d->mainWindow->isVisible() );
    }
}

void KCallApplication::onAccountManagerReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "Account manager failed to become ready:" << op->errorMessage();
        return; //TODO handle this error
    }

    foreach(const QString & a, d->accountManager->validAccountPaths()) {
        d->contactsModel->addAccount(d->accountManager->busName(), a);
    }

    connect(d->accountManager.data(), SIGNAL(accountCreated(QString)), SLOT(onAccountCreated(QString)));
}

void KCallApplication::onAccountCreated(const QString & path)
{
    d->contactsModel->addAccount(d->accountManager->busName(), path);
}

#include "kcallapplication.moc"
