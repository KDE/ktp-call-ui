/*
    Copyright (C) 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dialout-widget.h"
#include "ui_dialout-widget.h"

#include <KTp/Models/accounts-model.h>
#include <KTp/Models/accounts-model-item.h>
#include <KTp/Models/accounts-filter-model.h>

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/PendingReady>
#include <KDebug>

#define PREFERRED_CALL_HANDLER "org.freedesktop.Telepathy.Client.KTp.CallUi"

struct DialoutWidget::Private
{
    Ui::DialoutWidget *ui;
    AccountsModel *model;
    AccountsFilterModel *filterModel;
    Tp::AccountManagerPtr accountManager;
};

DialoutWidget::DialoutWidget(QWidget *parent)
    : QWidget(parent),
      d(new Private)
{
    d->ui = new Ui::DialoutWidget;
    d->ui->setupUi(this);

    d->model = new AccountsModel(this);
    d->filterModel = new AccountsFilterModel(this);
    d->filterModel->setSourceModel(d->model);
    d->filterModel->setPresenceTypeFilterFlags(AccountsFilterModel::ShowOnlyConnected);
    d->filterModel->setCapabilityFilterFlags(AccountsFilterModel::FilterByMediaCallCapability);
    d->ui->accountComboBox->setModel(d->filterModel);

    Tp::AccountFactoryPtr accountFactory = Tp::AccountFactory::create(
            QDBusConnection::sessionBus(),
            Tp::Features() << Tp::Account::FeatureCore
                           << Tp::Account::FeatureCapabilities);
    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
            QDBusConnection::sessionBus());
    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(
            QDBusConnection::sessionBus());
    Tp::ContactFactoryPtr contactFactory = Tp::ContactFactory::create();

    d->accountManager = Tp::AccountManager::create(accountFactory, connectionFactory,
                                                   channelFactory, contactFactory);
    connect(d->accountManager->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountManagerReady(Tp::PendingOperation*)));

    d->ui->stackedWidget->setCurrentWidget(d->ui->messagePage);
    d->ui->messageLabel->setText(i18n("Loading accounts..."));
}

DialoutWidget::~DialoutWidget()
{
    delete d->ui;
    delete d;
}

void DialoutWidget::onAccountManagerReady(Tp::PendingOperation* op)
{
    if (op->isError()) {
        kDebug() << "AM failed to become ready" << op->errorName() << op->errorMessage();
        d->ui->messageLabel->setText(i18n("Error: Failed to load accounts"));
        return;
    }

    d->model->setAccountManager(d->accountManager);
    d->ui->messageLabel->setText(i18n("No capable accounts found. Please make sure that "
                                      "you are online and that the accounts that you have "
                                      "configured support audio and/or video calls"));

    onRowsChanged();
    connect(d->filterModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(onRowsChanged()));
    connect(d->filterModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(onRowsChanged()));
}

void DialoutWidget::onRowsChanged()
{
    if (d->filterModel->rowCount() > 0) {
        d->ui->stackedWidget->setCurrentWidget(d->ui->callPage);
    } else {
        d->ui->stackedWidget->setCurrentWidget(d->ui->messagePage);
    }
}

void DialoutWidget::on_uriLineEdit_textChanged(const QString &text)
{
    d->ui->audioCallButton->setEnabled(!text.isEmpty());
    d->ui->videoCallButton->setEnabled(!text.isEmpty());
}

void DialoutWidget::on_audioCallButton_clicked()
{
    QModelIndex index = d->filterModel->index(d->ui->accountComboBox->currentIndex(), 0);
    QString contactId = d->ui->uriLineEdit->text();
    Tp::AccountPtr account = d->filterModel->data(index,
                AccountsModel::ItemRole).value<AccountsModelItem*>()->account();
    account->ensureAudioCall(contactId, QLatin1String("audio"),
            QDateTime::currentDateTime(), QLatin1String(PREFERRED_CALL_HANDLER));
}

void DialoutWidget::on_videoCallButton_clicked()
{
    QModelIndex index = d->filterModel->index(d->ui->accountComboBox->currentIndex(), 0);
    QString contactId = d->ui->uriLineEdit->text();
    Tp::AccountPtr account = d->filterModel->data(index,
                AccountsModel::ItemRole).value<AccountsModelItem*>()->account();
    account->ensureAudioVideoCall(contactId, QLatin1String("audio"), QLatin1String("video"),
            QDateTime::currentDateTime(), QLatin1String(PREFERRED_CALL_HANDLER));
}
