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

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/Account>
#include <TelepathyQt/AccountCapabilityFilter>
#include <TelepathyQt/AccountPropertyFilter>
#include <TelepathyQt/ContactCapabilities>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/AndFilter>
#include <TelepathyQt/OrFilter>


#include <KMessageBox>
#include <KDebug>

#include <KTp/actions.h>
#include <KTp/contact-factory.h>
#include <KTp/contact.h>

struct DialoutWidget::Private
{
    Ui::DialoutWidget *ui;
    Tp::AccountManagerPtr accountManager;

    QPointer<Tp::PendingContacts> pendingContact;
    Tp::AccountPtr currentAccount;
    KTp::ContactPtr currentContact;
};

DialoutWidget::DialoutWidget(const QString &number, QWidget *parent)
    : QWidget(parent),
      d(new Private)
{
    d->ui = new Ui::DialoutWidget;
    d->ui->setupUi(this);

    Tp::AccountFactoryPtr accountFactory = Tp::AccountFactory::create(
            QDBusConnection::sessionBus(),
            Tp::Features() << Tp::Account::FeatureCore
                           << Tp::Account::FeatureCapabilities);
    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
            QDBusConnection::sessionBus(),
            Tp::Features() << Tp::Connection::FeatureSelfContact);
    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(
            QDBusConnection::sessionBus());
    Tp::ContactFactoryPtr contactFactory = KTp::ContactFactory::create(
                Tp::Features() << Tp::Contact::FeatureCapabilities);

    d->accountManager = Tp::AccountManager::create(accountFactory, connectionFactory,
                                                   channelFactory, contactFactory);
    connect(d->accountManager->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountManagerReady(Tp::PendingOperation*)));

    d->ui->stackedWidget->setCurrentWidget(d->ui->messagePage);
    d->ui->messageLabel->setText(i18n("Loading accounts..."));

    d->ui->uriLineEdit->setText(number);
}

DialoutWidget::~DialoutWidget()
{
    Tp::AccountPtr account = d->ui->accountComboBox->currentAccount();
    if (account) {
        KGlobal::config()->group("DialoutWidget").writeEntry("LastSelectedAccount", account->uniqueIdentifier());
    }

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

    Tp::AccountPropertyFilterPtr isOnlineFilter = Tp::AccountPropertyFilter::create();
    isOnlineFilter->addProperty(QLatin1String("online"), true);

    Tp::AccountCapabilityFilterPtr streamedAudioCallFilter = Tp::AccountCapabilityFilter::create(
                Tp::RequestableChannelClassSpecList() << Tp::RequestableChannelClassSpec::RequestableChannelClassSpec::streamedMediaAudioCall());

    Tp::AccountCapabilityFilterPtr streamedVideoCallFilter = Tp::AccountCapabilityFilter::create(
                Tp::RequestableChannelClassSpecList() << Tp::RequestableChannelClassSpec::RequestableChannelClassSpec::streamedMediaVideoCall());

    //need both streamed audio and normal audio to match older and newer CM

    Tp::AccountCapabilityFilterPtr audioCallFilter = Tp::AccountCapabilityFilter::create(
                Tp::RequestableChannelClassSpecList() << Tp::RequestableChannelClassSpec::RequestableChannelClassSpec::audioCall());

    Tp::AccountCapabilityFilterPtr videoCallFilter = Tp::AccountCapabilityFilter::create(
                Tp::RequestableChannelClassSpecList() << Tp::RequestableChannelClassSpec::RequestableChannelClassSpec::videoCall());

    Tp::AccountFilterPtr capabilityFilter = Tp::OrFilter<Tp::Account>::create(QList<Tp::AccountFilterConstPtr>() << streamedAudioCallFilter
                                                                                                                 << streamedVideoCallFilter
                                                                                                                 << audioCallFilter
                                                                                                                 << videoCallFilter);
    
    Tp::AccountFilterConstPtr accountFilter = Tp::AndFilter<Tp::Account>::create(QList<Tp::AccountFilterConstPtr>() << isOnlineFilter << capabilityFilter);

    Tp::AccountSetPtr accountSet = d->accountManager->filterAccounts(accountFilter);
    d->ui->accountComboBox->setAccountSet(accountSet);

    if (accountSet->accounts().size() > 0) {
        d->ui->stackedWidget->setCurrentWidget(d->ui->callPage);
    } else {
        d->ui->stackedWidget->setCurrentWidget(d->ui->messagePage);
    }

    d->ui->messageLabel->setText(i18n("No capable accounts found. Please make sure that "
                                      "you are online and that the accounts that you have "
                                      "configured support audio and/or video calls"));

    QString lastAccount = KGlobal::config()->group("DialoutWidget").readEntry("LastSelectedAccount");
    d->ui->accountComboBox->setCurrentAccount(lastAccount);
}

void DialoutWidget::on_accountComboBox_currentIndexChanged(int currentIndex)
{
    Q_UNUSED(currentIndex);
    QString contactId = d->ui->uriLineEdit->text();
    Tp::AccountPtr account = d->ui->accountComboBox->currentAccount();

    d->ui->audioCallButton->setEnabled(false);
    d->ui->videoCallButton->setEnabled(false);


    if (!contactId.isEmpty() && !account.isNull()) {
        requestContact(account, contactId);
    }
}

void DialoutWidget::on_uriLineEdit_textChanged(const QString &text)
{
    Tp::AccountPtr account = d->ui->accountComboBox->currentAccount();

    d->ui->audioCallButton->setEnabled(false);
    d->ui->videoCallButton->setEnabled(false);

    if (!account.isNull() && !text.isEmpty()) {
        requestContact(account, text);
    }
}

void DialoutWidget::requestContact(const Tp::AccountPtr &account, const QString &contactId)
{
    d->currentAccount = account;
    d->pendingContact = account->connection()->contactManager()->contactsForIdentifiers(
            QStringList() << contactId, Tp::Contact::FeatureCapabilities);
    connect(d->pendingContact, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingContactFinished(Tp::PendingOperation*)));
}

void DialoutWidget::onPendingContactFinished(Tp::PendingOperation *op)
{
    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts*>(op);
    Q_ASSERT(pc);

    if (pc->isError()) {
        kDebug() << "Error getting contact:" << pc->errorName() << pc->errorMessage();
    }

    if (pc == d->pendingContact && !pc->isError() && pc->contacts().size() > 0) {
        d->currentContact = KTp::ContactPtr::qObjectCast(pc->contacts().at(0));

        d->ui->audioCallButton->setEnabled(d->currentContact->audioCallCapability());
        d->ui->videoCallButton->setEnabled(d->currentContact->videoCallCapability());
    }
}

void DialoutWidget::on_audioCallButton_clicked()
{
    Tp::PendingChannelRequest *pcr = KTp::Actions::startAudioCall(d->currentAccount, d->currentContact);
    connect(pcr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingChannelRequestFinished(Tp::PendingOperation*)));
}

void DialoutWidget::on_videoCallButton_clicked()
{
    Tp::PendingChannelRequest *pcr = KTp::Actions::startAudioVideoCall(d->currentAccount, d->currentContact);
    connect(pcr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingChannelRequestFinished(Tp::PendingOperation*)));
}

void DialoutWidget::onPendingChannelRequestFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        kDebug() << "Failed to start Call channel:" << op->errorName() << op->errorMessage();
        KMessageBox::sorry(this, i18n("Failed to start call."));
    }
}
