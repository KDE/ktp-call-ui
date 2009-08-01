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
#include "systrayicon.h"
#include "knotifyapprover.h"
#include "libkcallprivate/accountmanager.h"
#include <KDebug>
#include <TelepathyQt4/ClientRegistrar>

struct KCallApplication::Private
{
    QPointer<MainWindow> mainWindow;
    AccountManager *accountManager;
    Tp::ClientRegistrarPtr registrar;
    Tp::SharedPtr<SystrayIcon> systrayIcon;
    Tp::SharedPtr<KNotifyApprover> knotifyApprover;
};

KCallApplication::KCallApplication()
    : KUniqueApplication(), d(new Private)
{
    d->mainWindow = NULL;
    d->accountManager = new AccountManager(this);

    d->registrar = Tp::ClientRegistrar::create();
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

AccountManager *KCallApplication::accountManager() const
{
    return d->accountManager;
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

#include "kcallapplication.moc"
