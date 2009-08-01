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
#include <KSettings/Dialog>
#include <TelepathyQt4/Account>

Q_DECLARE_METATYPE(Tp::AccountPtr)

MainWindow::MainWindow()
    : KXmlGuiWindow(),
      ui(new Ui::MainWindow)
{
    setWindowTitle(i18nc("@title:window", "KCall"));

    QWidget *centralWidget = new QWidget(this);
    ui->setupUi(centralWidget);
    setCentralWidget(centralWidget);

    QAbstractItemModel *model = KCallApplication::instance()->accountManager()->contactsModel();
    ui->contactsTreeView->setModel(model);
    new ContactListController(ui->contactsTreeView, model);

    ui->accountComboBox->setModel(model);
    connect(ui->dialAudioButton, SIGNAL(clicked()), SLOT(onDialAudioButtonClicked()));
    connect(ui->dialVideoButton, SIGNAL(clicked()), SLOT(onDialVideoButtonClicked()));

    setupActions();
    setupGUI(QSize(340, 460));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupActions()
{
    KStandardAction::quit(KCallApplication::instance(), SLOT(quit()), actionCollection());
    KStandardAction::preferences(this, SLOT(showSettingsDialog()), actionCollection());
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
    int row = ui->accountComboBox->currentIndex();
    QString id = ui->contactHandleLineEdit->text();
    if ( row < 0  || id.isEmpty() ) {
        return;
    }

    QAbstractItemModel *model = ui->accountComboBox->model();
    Tp::AccountPtr account = model->index(row, 0).data(KCall::ObjectPtrRole).value<Tp::AccountPtr>();
    Q_ASSERT( !account.isNull() );

    QVariantMap request;
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".ChannelType",
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType", Tp::HandleTypeContact);
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetID", id);
    request.insert(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".FUTURE.InitialAudio", true);
    if ( useVideo ) {
        request.insert(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".FUTURE.InitialVideo", true);
    }
    account->ensureChannel(request, QDateTime::currentDateTime(),
                           "org.freedesktop.Telepathy.Client.kcall_handler");
}

#include "mainwindow.moc"
