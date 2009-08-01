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
class QAbstractItemModel;

class KCALLPRIVATE_EXPORT AccountManager : public QObject
{
    Q_OBJECT
public:
    explicit AccountManager(QObject *parent = 0);
    virtual ~AccountManager();

    QAbstractItemModel *accountsModel() const;
    QAbstractItemModel *contactsModel() const;

private:
    struct Private;
    Private *const d;
    Q_PRIVATE_SLOT(d, void onAccountManagerReady(Tp::PendingOperation *op))
    Q_PRIVATE_SLOT(d, void onAccountCreated(const QString & path))
};

#endif
