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
#include "contactlistcontroller.h"
#include "models/contactsmodel.h"
#include "models/accountitem.h" //for Q_DECLARE_METATYPE
#include "models/contactitem.h" //for Q_DECLARE_METATYPE
#include <QtGui/QAbstractItemView>
#include <KDebug>
#include <KIcon>
#include <KSelectAction>
#include <KMenu>
#include <KLocalizedString>
#include <TelepathyQt4/ReferencedHandles>

struct ContactListController::Private
{
    QAbstractItemView *view;
    QAbstractItemModel *model;
    KMenu *contactMenu;
    KMenu *accountMenu;
    QModelIndex currentIndex;
};

ContactListController::ContactListController(QAbstractItemView *view, QAbstractItemModel *model)
    : QObject(view), d(new Private)
{
    d->view = view;
    d->model = model;

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextMenuRequested(QPoint)));

    // contact menu
    d->contactMenu = new KMenu(view);
    KAction *callAction = new KAction(KIcon("voicecall"), i18nc("@action:inmenu", "Call"), d->contactMenu);
    connect(callAction, SIGNAL(triggered()), SLOT(callContact()));
    d->contactMenu->addAction(callAction);

    // account menu
    d->accountMenu = new KMenu(view);
    KSelectAction *setStatusAction = new KSelectAction(i18nc("@action:inmenu", "Status"), d->accountMenu);
    setStatusAction->addAction(KIcon("user-online"), i18nc("@action:inmenu", "Available"));
    setStatusAction->addAction(KIcon("user-away"), i18nc("@action:inmenu", "Away"));
    setStatusAction->addAction(KIcon("user-away"), i18nc("@action:inmenu", "Be right back"));
    setStatusAction->addAction(KIcon("user-busy"), i18nc("@action:inmenu", "Busy"));
    setStatusAction->addAction(KIcon("user-busy"), i18nc("@action:inmenu", "Do not distrurb"));
    setStatusAction->addAction(KIcon("user-away-extended"), i18nc("@action:inmenu", "Extended Away"));
    setStatusAction->addAction(KIcon("user-invisible"), i18nc("@action:inmenu", "Invisible"));
    setStatusAction->addAction(KIcon("user-offline"), i18nc("@action:inmenu", "Offline"));
    connect(setStatusAction, SIGNAL(triggered(int)), SLOT(setStatus(int)));
    d->accountMenu->addAction(setStatusAction);
}

ContactListController::~ContactListController()
{
    delete d;
}

void ContactListController::contextMenuRequested(const QPoint & pos)
{
    QModelIndex index = d->view->indexAt(pos);
    if ( !index.isValid() ) {
        return;
    }

    QByteArray type = index.data(ContactsModel::ItemTypeRole).toByteArray();
    d->currentIndex = index;
    if ( type == "contact" ) {
        d->contactMenu->popup(d->view->mapToGlobal(pos));
    } else if ( type == "account" ) {
        d->accountMenu->popup(d->view->mapToGlobal(pos));
    }
}

void ContactListController::callContact()
{
    Q_ASSERT(d->currentIndex.isValid());
    Tp::ContactPtr contact = d->currentIndex.data(ContactsModel::ObjectPtrRole).value<Tp::ContactPtr>();
    Q_ASSERT( !contact.isNull() );

    //TODO this should be removed when Tp::Contact has support for requesting channels directly
    Tp::AccountPtr account = d->currentIndex.parent().data(ContactsModel::ObjectPtrRole).value<Tp::AccountPtr>();
    Q_ASSERT( !account.isNull() );

    QVariantMap request;
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".ChannelType",
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType", Tp::HandleTypeContact);
    request.insert(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle", contact->handle()[0]);
    request.insert(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".FUTURE.InitialAudio", true);
    account->ensureChannel(request, QDateTime::currentDateTime(), "org.freedesktop.Telepathy.Client.kcall");
}

void ContactListController::setStatus(int statusIndex)
{
    static const char *statuses[] = { "available", "away", "brb", "busy", "dnd", "xa", "hidden", "offline" };
    static Tp::ConnectionPresenceType types[] =
                        { Tp::ConnectionPresenceTypeAvailable, Tp::ConnectionPresenceTypeAway,
                          Tp::ConnectionPresenceTypeAway, Tp::ConnectionPresenceTypeBusy,
                          Tp::ConnectionPresenceTypeBusy, Tp::ConnectionPresenceTypeExtendedAway,
                          Tp::ConnectionPresenceTypeHidden, Tp::ConnectionPresenceTypeOffline };

    Q_ASSERT(statusIndex >= 0 && statusIndex <= 7);
    Tp::SimplePresence presence;
    presence.type = types[statusIndex];
    presence.status = QLatin1String(statuses[statusIndex]);

    Q_ASSERT(d->currentIndex.isValid());
    Tp::AccountPtr account = d->currentIndex.data(ContactsModel::ObjectPtrRole).value<Tp::AccountPtr>();
    Q_ASSERT( !account.isNull() );

    account->setRequestedPresence(presence);
}

#include "contactlistcontroller.moc"
