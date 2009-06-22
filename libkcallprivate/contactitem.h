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
#ifndef CONTACTITEM_H
#define CONTACTITEM_H

#include "contactsmodel.h"
#include <TelepathyQt4/Contact>

class ContactItem : public ContactsModelItem
{
public:
    ContactItem(const Tp::ContactPtr & contact, TreeModelItem *parent, TreeModel *model);

    virtual QVariant data(int role) const;

private:
    Tp::ContactPtr m_contact;
};

Q_DECLARE_METATYPE(Tp::ContactPtr);

#endif
