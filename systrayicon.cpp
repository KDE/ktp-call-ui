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
#include "systrayicon.h"
#include "kcallapplication.h"
#include "mainwindow.h"
#include <QtCore/QSet>
#include <KLocalizedString>
#include <KDebug>
#include <knotificationitem.h>
#include <TelepathyQt4/Constants>

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

struct SystrayIcon::Private
{
    QSet<ApproverRequest*> requests;
    QPointer<ApproverRequest> currentRequest;
    Experimental::KNotificationItem *notificationItem;
};

SystrayIcon::SystrayIcon()
    : AbstractClientApprover(channelClassList()), d(new Private)
{
    d->notificationItem = new Experimental::KNotificationItem(this);
    d->notificationItem->setTitle(i18n("KCall"));
    d->notificationItem->setIconByName("internet-telephony");
    d->notificationItem->setAttentionIconByName("voicecall");
    d->notificationItem->setCategory(Experimental::KNotificationItem::Communications);
    d->notificationItem->setStatus(Experimental::KNotificationItem::Active);

    connect(d->notificationItem, SIGNAL(activateRequested(bool, QPoint)),
            SLOT(onActivateRequested(bool, QPoint)));

    KCallApplication::instance()->mainWindow()->installEventFilter(this);
}

SystrayIcon::~SystrayIcon()
{
    delete d;
}

void SystrayIcon::newRequest(ApproverRequest *request)
{
    d->requests.insert(request);
    d->currentRequest = request;
    d->notificationItem->setStatus(Experimental::KNotificationItem::NeedsAttention);
}

void SystrayIcon::requestFinished(ApproverRequest *request)
{
    d->requests.remove(request);
    kDebug() << d->requests.size();
    if ( d->requests.size() == 0 ) {
        d->notificationItem->setStatus(Experimental::KNotificationItem::Passive);
    }
}

void SystrayIcon::onActivateRequested(bool active, const QPoint & pos)
{
    Q_UNUSED(active);
    Q_UNUSED(pos);
    if ( d->currentRequest ) {
        d->currentRequest->accept();
    } else {
        MainWindow *mw = KCallApplication::instance()->mainWindow();
        //reinstall the event filter because the mainwindow is deleted on close, so in some cases
        //a new mainwindow is returned here.
        mw->removeEventFilter(this);
        mw->installEventFilter(this);
        if ( !mw->isVisible() ) {
            mw->show();
            mw->raise();
            mw->activateWindow();
        } else {
            mw->hide();
        }
    }
}

bool SystrayIcon::eventFilter(QObject *watched, QEvent *event)
{
    if ( watched != KCallApplication::instance()->mainWindow() ) {
        return AbstractClientApprover::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::Show:
        d->notificationItem->setStatus(Experimental::KNotificationItem::Passive);
        break;
    case QEvent::Hide:
    case QEvent::Close:
        d->notificationItem->setStatus(Experimental::KNotificationItem::Active);
        break;
    default:
        break;
    }
    return false;
}

#include "systrayicon.moc"
