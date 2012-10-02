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

#include <KTp/Models/contacts-model.h>
#include <KTp/Models/accounts-filter-model.h>

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/ContactCapabilities>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingReady>

#include <KMessageBox>
#include <KDebug>

#define PREFERRED_CALL_HANDLER "org.freedesktop.Telepathy.Client.KTp.CallUi"

namespace CapabilitiesHackPrivate {

/*
 * This is a hack to workaround a gabble bug.
 * https://bugs.freedesktop.org/show_bug.cgi?id=51978
 */

Tp::RequestableChannelClassSpec gabbleAudioCallRCC()
{
    static Tp::RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        Tp::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) Tp::HandleTypeContact);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName"));
        spec = Tp::RequestableChannelClassSpec(rcc);
    }

    return spec;
}

Tp::RequestableChannelClassSpec gabbleAudioVideoCallRCC()
{
    static Tp::RequestableChannelClassSpec spec;

    if (!spec.isValid()) {
        Tp::RequestableChannelClass rcc;
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                TP_QT_IFACE_CHANNEL_TYPE_CALL);
        rcc.fixedProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                (uint) Tp::HandleTypeContact);
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudioName"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"));
        rcc.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideoName"));
        spec = Tp::RequestableChannelClassSpec(rcc);
    }

    return spec;
}

bool audioCalls(const Tp::CapabilitiesBase &caps, const QString &cmName)
{
    bool gabbleResult = false;
    if (cmName == QLatin1String("gabble")) {
        Q_FOREACH (const Tp::RequestableChannelClassSpec &rccSpec, caps.allClassSpecs()) {
            if (rccSpec.supports(gabbleAudioCallRCC())) {
                gabbleResult = true;
                break;
            }
        }
    }

    return gabbleResult || caps.audioCalls();
}

bool audioVideoCalls(const Tp::CapabilitiesBase &caps, const QString &cmName)
{
    bool gabbleResult = false;
    if (cmName == QLatin1String("gabble")) {
        Q_FOREACH (const Tp::RequestableChannelClassSpec &rccSpec, caps.allClassSpecs()) {
            if (rccSpec.supports(gabbleAudioVideoCallRCC())) {
                gabbleResult = true;
                break;
            }
        }
    }

    return gabbleResult || caps.videoCalls();
}

} //namespace CapabilitiesHackPrivate


struct DialoutWidget::Private
{
    Ui::DialoutWidget *ui;
    ContactsModel *model;
    AccountsFilterModel *filterModel;
    Tp::AccountManagerPtr accountManager;

    QPointer<Tp::PendingContacts> pendingContact;
    Tp::AccountPtr currentAccount;
    Tp::ContactPtr currentContact;
};

DialoutWidget::DialoutWidget(const QString &number, QWidget *parent)
    : QWidget(parent),
      d(new Private)
{
    d->ui = new Ui::DialoutWidget;
    d->ui->setupUi(this);

    d->model = new ContactsModel(this);
    d->filterModel = new AccountsFilterModel(this);
    d->filterModel->setSourceModel(d->model);
    d->filterModel->setPresenceTypeFilterFlags(AccountsFilterModel::ShowOnlyConnected);
    d->filterModel->setCapabilityFilterFlags(AccountsFilterModel::FilterByMediaCallCapability);
    d->filterModel->setDynamicSortFilter(true);
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

    d->ui->uriLineEdit->setText(number);
}

DialoutWidget::~DialoutWidget()
{
    int currentIndex = d->ui->accountComboBox->currentIndex();
    if (currentIndex >= 0) {
        KGlobal::config()->group("DialoutWidget").writeEntry("LastSelectedAccountIndex", currentIndex);
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

    d->model->setAccountManager(d->accountManager);
    d->ui->messageLabel->setText(i18n("No capable accounts found. Please make sure that "
                                      "you are online and that the accounts that you have "
                                      "configured support audio and/or video calls"));

    onRowsChanged();
    connect(d->filterModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(onRowsChanged()));
    connect(d->filterModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(onRowsChanged()));

    int savedIndex = KGlobal::config()->group("DialoutWidget").readEntry("LastSelectedAccountIndex", 0);
    if (d->filterModel->rowCount() > 0 && savedIndex < d->filterModel->rowCount()) {
        d->ui->accountComboBox->setCurrentIndex(savedIndex);
    }
}

void DialoutWidget::onRowsChanged()
{
    if (d->filterModel->rowCount() > 0) {
        d->ui->stackedWidget->setCurrentWidget(d->ui->callPage);
    } else {
        d->ui->stackedWidget->setCurrentWidget(d->ui->messagePage);
    }
}

void DialoutWidget::on_accountComboBox_currentIndexChanged(int currentIndex)
{
    QString contactId = d->ui->uriLineEdit->text();

    d->ui->audioCallButton->setEnabled(false);
    d->ui->videoCallButton->setEnabled(false);

    if (!contactId.isEmpty() && currentIndex != -1) {
        QModelIndex index = d->filterModel->index(currentIndex, 0);
        Tp::AccountPtr account = d->filterModel->data(index,ContactsModel::AccountRole).value<Tp::AccountPtr>();
        requestContact(account, contactId);
    }
}

void DialoutWidget::on_uriLineEdit_textChanged(const QString &text)
{
    QModelIndex index = d->filterModel->index(d->ui->accountComboBox->currentIndex(), 0);

    d->ui->audioCallButton->setEnabled(false);
    d->ui->videoCallButton->setEnabled(false);

    if (index.isValid() && !text.isEmpty()) {
        Tp::AccountPtr account = d->filterModel->data(index,ContactsModel::AccountRole).value<Tp::AccountPtr>();
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
        d->currentContact = pc->contacts().at(0);
        d->ui->audioCallButton->setEnabled(CapabilitiesHackPrivate::audioCalls(
                d->currentContact->capabilities(), pc->manager()->connection()->cmName()));
        d->ui->videoCallButton->setEnabled(CapabilitiesHackPrivate::audioVideoCalls(
                d->currentContact->capabilities(), pc->manager()->connection()->cmName()));
    }
}

void DialoutWidget::on_audioCallButton_clicked()
{
    Tp::PendingChannelRequest *pcr =
        d->currentAccount->ensureAudioCall(d->currentContact,
            QLatin1String("audio"),
            QDateTime::currentDateTime(),
            QLatin1String(PREFERRED_CALL_HANDLER));

    connect(pcr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingChannelRequestFinished(Tp::PendingOperation*)));
}

void DialoutWidget::on_videoCallButton_clicked()
{
    Tp::PendingChannelRequest *pcr =
        d->currentAccount->ensureAudioVideoCall(d->currentContact,
            QLatin1String("audio"),
            QLatin1String("video"),
            QDateTime::currentDateTime(),
            QLatin1String(PREFERRED_CALL_HANDLER));

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
