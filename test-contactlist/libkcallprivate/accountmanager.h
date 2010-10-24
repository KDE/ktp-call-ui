/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include "kcallprivate_export.h"
#include <QtCore/QObject>
#include <TelepathyQt4/Constants>
class QAbstractItemModel;

/** This class handles connection with the account manager, constructs models
 * for showing all the available accounts and contacts and offers some global
 * connection status tracking.
 */
class KCALLPRIVATE_EXPORT AccountManager : public QObject
{
    Q_OBJECT
public:
    explicit AccountManager(QObject *parent = 0);
    virtual ~AccountManager();

    /** Returns a model that lists all the available accounts. */
    QAbstractItemModel *accountsModel() const;

    /** Returns a model that lists all the available contacts.
     * Currently this is the same as accountsModel() and returns a tree model
     * showing the accounts in the first level and the contacts for each
     * account in the second level.
     * @todo Write a proper contacts model.
     */
    QAbstractItemModel *contactsModel() const;

    /** Returns the global connection status. This is "Offline" if all accounts are offline,
     * "Connecting" if at least one account is connecting and all others are offline and
     * "Connected" if at least one account is connected.
     */
    Tp::ConnectionStatus globalConnectionStatus() const;

public Q_SLOTS:
    /** Attempts to put all the enabled accounts online, by setting their
     * "RequestedPresence" property to the value of their "AutomaticPresence" property.
     */
    void connectAccounts();

    /** Attempts to disconnect all the enabled accounts by setting their "RequestedPresence"
     * property to Tp::ConnectionPresenceTypeOffline/"offline".
     */
    void disconnectAccounts();

Q_SIGNALS:
    /** This is emmited when the value of globalConnectionStatus() changes. */
    void globalConnectionStatusChanged(Tp::ConnectionStatus status);

private:
    struct Private;
    Private *const d;
    Q_PRIVATE_SLOT(d, void onAccountManagerReady(Tp::PendingOperation *op))
    Q_PRIVATE_SLOT(d, void onAccountCreated(const QString & path))
    Q_PRIVATE_SLOT(d, void onAccountReady(Tp::PendingOperation *op))
    Q_PRIVATE_SLOT(d, void onAccountRemoved(const QString & path))
    Q_PRIVATE_SLOT(d, void onAccountValidityChanged(const QString & path, bool isValid))
    Q_PRIVATE_SLOT(d, void onAccountConnectionStatusChanged(Tp::ConnectionStatus,
                                                            Tp::ConnectionStatusReason))
};

#endif
