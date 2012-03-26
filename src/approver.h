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
#ifndef APPROVER_H
#define APPROVER_H

#include <QtCore/QSharedPointer>
#include <TelepathyQt/CallChannel>

class KStatusNotifierItem;
class KNotification;

class Approver : public QObject
{
    Q_OBJECT
public:
    Approver(const Tp::CallChannelPtr & channel, QObject *parent);
    virtual ~Approver();

Q_SIGNALS:
    void channelAccepted();
    void channelRejected();

private:
    QWeakPointer<KNotification> m_notification;
    KStatusNotifierItem *m_notifierItem;
};

#endif //APPROVER_H
