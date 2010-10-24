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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "kcallapplication.h"
#include "contactlistcontroller.h"
#include "libkcallprivate/accountmanager.h"
#include "libkcallprivate/constants.h"
#include <KStatusBar>
#include <KAction>
#include <KActionCollection>
#include <KMessageBox>
#include <KSettings/Dialog>
#include <TelepathyQt4/Account>

Q_DECLARE_METATYPE(Tp::AccountPtr)

struct MainWindow::Private
{
    Ui::MainWindow ui;
    KAction *goOnlineAction;
};

MainWindow::MainWindow()
    : KXmlGuiWindow(),
      d(new Private)
{
    setWindowTitle(i18nc("@title:window", "KCall"));

    QWidget *centralWidget = new QWidget(this);
    d->ui.setupUi(centralWidget);
    setCentralWidget(centralWidget);

    QAbstractItemModel *model = KCallApplication::instance()->accountManager()->contactsModel();
    d->ui.contactsTreeView->setModel(model);
    new ContactListController(d->ui.contactsTreeView, model);

    d->ui.accountComboBox->setModel(model);
    connect(d->ui.dialAudioButton, SIGNAL(clicked()), SLOT(onDialAudioButtonClicked()));
    connect(d->ui.dialVideoButton, SIGNAL(clicked()), SLOT(onDialVideoButtonClicked()));

    setupActions();
    setupGUI(QSize(340, 460));
}

MainWindow::~MainWindow()
{
    delete d;
}

void MainWindow::setupActions()
{
    KStandardAction::quit(KCallApplication::instance(), SLOT(quit()), actionCollection());
    KStandardAction::preferences(this, SLOT(showSettingsDialog()), actionCollection());

    d->goOnlineAction = new KAction(this);
    d->goOnlineAction->setIcon(KIcon("network-connect"));
    actionCollection()->addAction("go-online", d->goOnlineAction);

    AccountManager *accountManager = KCallApplication::instance()->accountManager();
    onGlobalConnectionStatusChanged(accountManager->globalConnectionStatus());

    connect(d->goOnlineAction, SIGNAL(triggered()), this, SLOT(onGoOnlineTriggered()));
    connect(accountManager, SIGNAL(globalConnectionStatusChanged(Tp::ConnectionStatus)),
            this, SLOT(onGlobalConnectionStatusChanged(Tp::ConnectionStatus)));
}

void MainWindow::onGoOnlineTriggered()
{
    switch ( KCallApplication::instance()->accountManager()->globalConnectionStatus() ) {
    case Tp::ConnectionStatusDisconnected:
        KCallApplication::instance()->accountManager()->connectAccounts();
        break;
    default:
        KCallApplication::instance()->accountManager()->disconnectAccounts();
        break;
    }
}

void MainWindow::onGlobalConnectionStatusChanged(Tp::ConnectionStatus status)
{
    switch (status) {
    case Tp::ConnectionStatusConnected:
        statusBar()->showMessage(i18nc("@info:status", "Connected"));
        d->goOnlineAction->setText(i18nc("@action", "Disconnect"));
        break;
    case Tp::ConnectionStatusConnecting:
        statusBar()->showMessage(i18nc("@info:status", "Connecting..."));
        d->goOnlineAction->setText(i18nc("@action", "Cancel connection attempt"));
        break;
    case Tp::ConnectionStatusDisconnected:
        statusBar()->showMessage(i18nc("@info:status", "Disconnected"));
        d->goOnlineAction->setText(i18nc("@action", "Connect"));
        break;
    default:
        Q_ASSERT(false);
    }
}

void MainWindow::showSettingsDialog()
{
    KSettings::Dialog *dialog = new KSettings::Dialog(this);
    dialog->addModule("kcm_telepathy_accounts");
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::onDialAudioButtonClicked()
{
    makeDirectCall(false);
}

void MainWindow::onDialVideoButtonClicked()
{
    makeDirectCall(true);
}

void MainWindow::makeDirectCall(bool useVideo)
{
    int row = d->ui.accountComboBox->currentIndex();
    QString id = d->ui.contactHandleLineEdit->text();
    if ( row < 0  || id.isEmpty() ) {
        return;
    }

    QAbstractItemModel *model = d->ui.accountComboBox->model();
    Tp::AccountPtr account = model->index(row, 0).data(KCall::ObjectPtrRole).value<Tp::AccountPtr>();
    Q_ASSERT( !account.isNull() );

    QVariantMap request;
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".ChannelType",
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType", Tp::HandleTypeContact);
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetID", id);
    request.insert(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialAudio", true);
    if ( useVideo ) {
        request.insert(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialVideo", true);
    }
    account->ensureChannel(request, QDateTime::currentDateTime(),
                           "org.freedesktop.Telepathy.Client.kcall_handler");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    KMessageBox::information(this, i18n("You have requested to close KCall. Note that KCall will "
                                        "not actually quit, but it will stay active in the system "
                                        "tray waiting for incoming calls."),
                             QString(), QLatin1String("CloseEventTrayNotification"));
    KXmlGuiWindow::closeEvent(event);
}

#include "mainwindow.moc"
