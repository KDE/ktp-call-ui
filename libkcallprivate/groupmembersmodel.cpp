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
#include "groupmembersmodel.h"
#include "contactitem.h"

class GroupContactItem : public ContactItem
{
public:
    inline GroupContactItem(const Tp::ContactPtr & contact, KCall::GroupMembersListType listType,
                            TreeModelItem *parent, TreeModel *model)
        : ContactItem(contact, parent, model), m_listType(listType)
    {
    }

    virtual QVariant data(int role) const
    {
        switch(role) {
        case KCall::GroupMembersListTypeRole:
            return QVariant::fromValue(m_listType);
        default:
            return ContactItem::data(role);
        }
    }

    inline void setListType(KCall::GroupMembersListType listType)
    {
        if ( m_listType != listType ) {
            m_listType = listType;
            emitDataChange();
        }
    }

private:
    KCall::GroupMembersListType m_listType;
};


struct GroupMembersModel::Private
{
    Tp::ChannelPtr channel;
    QHash<QString, GroupContactItem*> contactItems;
};


GroupMembersModel::GroupMembersModel(const Tp::ChannelPtr & channel, QObject *parent)
    : TreeModel(parent), d(new Private)
{
    if ( channel->interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_GROUP) ) {
        d->channel = channel;
        foreach (const Tp::ContactPtr & contact, d->channel->groupContacts()) {
            addContact(contact, KCall::CurrentMembers);
        }
        foreach (const Tp::ContactPtr & contact, d->channel->groupLocalPendingContacts()) {
            addContact(contact, KCall::LocalPendingMembers);
        }
        foreach (const Tp::ContactPtr & contact, d->channel->groupRemotePendingContacts()) {
            addContact(contact, KCall::RemotePendingMembers);
        }

        connect(d->channel.data(),
                SIGNAL(groupMembersChanged(Tp::Contacts, Tp::Contacts, Tp::Contacts, Tp::Contacts,
                                            Tp::Channel::GroupMemberChangeDetails)),
                SLOT(onGroupMembersChanged(Tp::Contacts, Tp::Contacts, Tp::Contacts, Tp::Contacts,
                                            Tp::Channel::GroupMemberChangeDetails)));
    }
}

GroupMembersModel::~GroupMembersModel()
{
    delete d;
}

void GroupMembersModel::addContact(const Tp::ContactPtr & contact,
                                        KCall::GroupMembersListType listType)
{
    if ( d->contactItems.contains(contact->id()) ) {
        d->contactItems[contact->id()]->setListType(listType);
    } else {
        GroupContactItem *item = new GroupContactItem(contact, listType, root(), this);
        d->contactItems[contact->id()] = item;
        root()->appendChild(item);
    }
}

void GroupMembersModel::removeContact(const Tp::ContactPtr & contact)
{
    Q_ASSERT(d->contactItems.contains(contact->id()));
    root()->removeChild(root()->rowOfChild(d->contactItems.take(contact->id())));
}

void GroupMembersModel::onGroupMembersChanged(const Tp::Contacts & groupMembersAdded,
                                            const Tp::Contacts & groupLocalPendingMembersAdded,
                                            const Tp::Contacts & groupRemotePendingMembersAdded,
                                            const Tp::Contacts & groupMembersRemoved,
                                            const Tp::Channel::GroupMemberChangeDetails & details)
{
    Q_UNUSED(details);

    foreach (const Tp::ContactPtr & contact, groupMembersAdded) {
        addContact(contact, KCall::CurrentMembers);
    }
    foreach (const Tp::ContactPtr & contact, groupLocalPendingMembersAdded) {
        addContact(contact, KCall::LocalPendingMembers);
    }
    foreach (const Tp::ContactPtr & contact, groupRemotePendingMembersAdded) {
        addContact(contact, KCall::RemotePendingMembers);
    }
    foreach (const Tp::ContactPtr & contact, groupMembersRemoved) {
        removeContact(contact);
    }
}

#include "groupmembersmodel.moc"
