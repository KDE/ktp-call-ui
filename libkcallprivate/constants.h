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
#ifndef _KCALL_CONSTANTS_H
#define _KCALL_CONSTANTS_H

#include <QtCore/QtGlobal>
#include <QtCore/QMetaType>

namespace KCall
{
    enum ExtraModelRoles {
        ItemTypeRole = Qt::UserRole,
        ObjectPtrRole,
        GroupMembersListTypeRole,
        PresenceRole
    };

    enum GroupMembersListType {
        CurrentMembers,
        LocalPendingMembers,
        RemotePendingMembers
    };
}

Q_DECLARE_METATYPE(KCall::GroupMembersListType)

#endif
