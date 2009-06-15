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
#include "knotifyapprover.h"
#include <QtCore/QHash>
#include <KDebug>
#include <KNotification>
#include <KLocalizedString>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelDispatchOperation>

static inline Tp::ChannelClassList channelClassList()
{
    QMap<QString, QDBusVariant> class1;
    class1[TELEPATHY_INTERFACE_CHANNEL ".ChannelType"] =
                                    QDBusVariant(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    class1[TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"] = QDBusVariant(Tp::HandleTypeContact);

    QMap<QString, QDBusVariant> class2;
    class2[TELEPATHY_INTERFACE_CHANNEL ".ChannelType"] =
                                    QDBusVariant(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    class2[TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"] = QDBusVariant(Tp::HandleTypeContact);
    class2[TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".FUTURE.InitialAudio"] = QDBusVariant(true);

    return Tp::ChannelClassList() << Tp::ChannelClass(class1) << Tp::ChannelClass(class2);
}

struct KNotifyApprover::Private
{
    QHash<ApproverRequest*, QPointer<KNotification> > notifications;
};

KNotifyApprover::KNotifyApprover()
    : AbstractClientApprover(channelClassList()), d(new Private)
{
}

KNotifyApprover::~KNotifyApprover()
{
    delete d;
}

void KNotifyApprover::newRequest(ApproverRequest *request)
{
    Tp::ContactPtr contact = request->dispatchOperation()->channels()[0]->initiatorContact();

    KNotification *notification = new KNotification("newcall", NULL, KNotification::Persistent);
    notification->setText( i18n("%1 is calling you", contact->alias()) );
    notification->setActions( QStringList() << i18n("Answer") << i18n("Ignore") );

    connect(notification, SIGNAL(action1Activated()), request, SLOT(accept()));
    connect(notification, SIGNAL(action2Activated()), request, SLOT(reject()));
    connect(notification, SIGNAL(ignored()), request, SLOT(reject()));
    notification->sendEvent();

    d->notifications[request] = notification;
}

void KNotifyApprover::requestFinished(ApproverRequest *request)
{
    if ( d->notifications[request] ) {
        kDebug() << "Closing notification";
        d->notifications[request]->close();
    }
    d->notifications.remove(request);
}

#include "knotifyapprover.moc"
