/*
    Copyright (C) 2010-2011 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "approver.h"
#include "ktp_call_ui_debug.h"

#include <KNotification>
#include <KStatusNotifierItem>
#include <KLocalizedString>
#include <KAboutData>
#include <KActionCollection>
#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Contact>

#include <QMenu>

Approver::Approver(const Tp::CallChannelPtr & channel, QObject *parent)
    : QObject(parent)
{
    qCDebug(KTP_CALL_UI);

    Tp::ContactPtr sender = channel->initiatorContact();
    QString message;
    QString title;
    if (channel->hasInitialVideo()) {
        title = i18n("Incoming video call");
        message = i18n("<p>%1 is asking you for a video call</p>", sender->alias());
    } else {
        title = i18n("Incoming call");
        message = i18n("<p>%1 is calling you</p>", sender->alias());
    }

    //notification
    m_notification = new KNotification("incoming_call");
    m_notification.data()->setComponentName("ktelepathy");
    m_notification.data()->setTitle(title);
    m_notification.data()->setText(message);

    QPixmap pixmap;
    if (pixmap.load(sender->avatarData().fileName)) {
        m_notification.data()->setPixmap(pixmap);
    }

    m_notification.data()->setActions(QStringList()
            << i18nc("action, answers the incoming call", "Answer")
            << i18nc("action, rejects the incoming call", "Reject"));
    connect(m_notification.data(), SIGNAL(action1Activated()), SIGNAL(channelAccepted()));
    connect(m_notification.data(), SIGNAL(action2Activated()), SIGNAL(channelRejected()));

    m_notification.data()->sendEvent();

    //tray icon
    m_notifierItem = new KStatusNotifierItem;
    m_notifierItem->setCategory(KStatusNotifierItem::Communications);
    m_notifierItem->setStatus(KStatusNotifierItem::NeedsAttention);
    m_notifierItem->setIconByName(QLatin1String("internet-telephony"));

    if (channel->hasInitialVideo()) {
        m_notifierItem->setAttentionIconByName(QLatin1String("camera-web"));
    } else {
        m_notifierItem->setAttentionIconByName(QLatin1String("audio-headset"));
    }

    m_notifierItem->setStandardActionsEnabled(false);
    m_notifierItem->setTitle(title);
    m_notifierItem->setToolTip(QLatin1String("internet-telephony"), message, QString());
    m_notifierItem->contextMenu()->addAction(i18nc("action, answers the incoming call", "Answer"),
                                             this, SIGNAL(channelAccepted()));
    m_notifierItem->contextMenu()->addAction(i18nc("action, rejects the incoming call", "Reject"),
                                             this, SIGNAL(channelRejected()));
    connect(m_notifierItem, SIGNAL(activateRequested(bool,QPoint)), SIGNAL(channelAccepted()));
}

Approver::~Approver()
{
    qCDebug(KTP_CALL_UI);

    //destroy the notification
    if (m_notification) {
        m_notification.data()->close();
    }

    //destroy the tray icon
    delete m_notifierItem;
}

#include "moc_approver.cpp"
