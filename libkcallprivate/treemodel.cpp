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
#include "treemodel.h"
#include <QtCore/QVector>

//BEGIN TreeModelItem

struct TreeModelItem::Private
{
    TreeModel *model;
    TreeModelItem *parent;
    QList<TreeModelItem*> children;
};

TreeModelItem::TreeModelItem(TreeModelItem *parent, TreeModel *model)
    : d(new Private)
{
    d->parent = parent;
    d->model = model;
}

TreeModelItem::~TreeModelItem()
{
    qDeleteAll(d->children);
    delete d;
}

QVariant TreeModelItem::data(int role) const
{
    Q_UNUSED(role);
    return QVariant();
}

TreeModelItem *TreeModelItem::parentItem() const
{
    return d->parent;
}

TreeModel *TreeModelItem::model() const
{
    return d->model;
}

QModelIndex TreeModelItem::index()
{
    if (!d->parent) {
        return QModelIndex();
    } else {
        return d->model->createIndex(d->parent->rowOfChild(this), 1, static_cast<void*>(this));
    }
}

bool TreeModelItem::hasChildren() const
{
    return d->children.size() > 0;
}

int TreeModelItem::childrenCount() const
{
    return d->children.size();
}

TreeModelItem *TreeModelItem::childItem(int row) const
{
    return d->children.at(row);
}

int TreeModelItem::rowOfChild(TreeModelItem *child) const
{
    Q_ASSERT(child->parentItem() == this);
    return d->children.indexOf(child);
}


void TreeModelItem::appendChild(TreeModelItem *child)
{
    Q_ASSERT(child->parentItem() == this);
    d->model->beginInsertRows(index(), d->children.size(), d->children.size());
    d->children.append(child);
    d->model->endInsertRows();
}

void TreeModelItem::appendChildren(const QVector<TreeModelItem*> & children)
{
    if (children.size() == 0) {
        return;
    }

    d->model->beginInsertRows(index(), d->children.size(), d->children.size() + children.size() - 1);
    foreach(TreeModelItem *child, children) {
        Q_ASSERT(child->parentItem() == this);
        d->children.append(child);
    }
    d->model->endInsertRows();
}

void TreeModelItem::removeChild(int row)
{
    Q_ASSERT(row >= 0 && row < d->children.size());

    d->model->beginRemoveRows(index(), row, row);
    TreeModelItem *child = d->children.takeAt(row);
    delete child;
    d->model->endRemoveRows();
}

void TreeModelItem::removeChildren(int first, int last)
{
    Q_ASSERT(first >= 0 && last < d->children.size() && first <= last);

    d->model->beginRemoveRows(index(), first, last);
    for(int i=first; i<=last; i++) {
        TreeModelItem *child = d->children.takeAt(first);
        delete child;
    }
    d->model->endRemoveRows();
}

void TreeModelItem::emitDataChange()
{
    QModelIndex selfIndex = index();
    emit d->model->dataChanged(selfIndex, selfIndex);
}

//END TreeModelItem

//BEGIN TreeModel

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_root = new TreeModelItem(NULL, this);
}

TreeModel::~TreeModel()
{
    delete m_root;
}

QVariant TreeModel::data(const QModelIndex & index, int role) const
{
    if (index.isValid()) {
        return static_cast<TreeModelItem*>(index.internalPointer())->data(role);
    } else {
        return QVariant();
    }
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex & parent) const
{
    if ( !hasIndex(row, column, parent) ) {
        return QModelIndex();
    }

    TreeModelItem *parentItem = m_root;
    if ( parent.isValid() ) {
        parentItem = static_cast<TreeModelItem*>(parent.internalPointer());
    }

    if ( row >= parentItem->childrenCount() ) {
        return QModelIndex();
    } else {
        return createIndex(row, column, static_cast<void*>(parentItem->childItem(row)));
    }
}

QModelIndex TreeModel::parent(const QModelIndex & index) const
{
    if ( !index.isValid() ) {
        return QModelIndex();
    }

    TreeModelItem *item = static_cast<TreeModelItem*>(index.internalPointer());
    Q_ASSERT(item != m_root);
    return item->parentItem()->index();
}

int TreeModel::rowCount(const QModelIndex & parent) const
{
    if ( parent.isValid() ) {
        TreeModelItem *item = static_cast<TreeModelItem*>(parent.internalPointer());
        return item->childrenCount();
    } else {
        return m_root->childrenCount();
    }
}

int TreeModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool TreeModel::hasChildren(const QModelIndex & parent) const
{
    if ( !parent.isValid() ) {
        return m_root->hasChildren();
    } else {
        TreeModelItem *item = static_cast<TreeModelItem*>(parent.internalPointer());
        return item->hasChildren();
    }
}

//END TreeModel

#include "treemodel.moc"
