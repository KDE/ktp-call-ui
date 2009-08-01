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
#ifndef TREEMODEL_H
#define TREEMODEL_H

#include "kcallprivate_export.h"
#include <QtCore/QAbstractItemModel>
class TreeModel;

class TreeModelItem
{
    Q_DISABLE_COPY(TreeModelItem);
public:
    TreeModelItem(TreeModel *model);
    TreeModelItem(TreeModelItem *parent);
    virtual ~TreeModelItem();

    virtual QVariant data(int role) const;

    TreeModelItem *parentItem() const;
    TreeModel *model() const;
    QModelIndex index();

    bool hasChildren() const;
    int childrenCount() const;
    TreeModelItem *childItem(int row) const;
    int rowOfChild(TreeModelItem *child) const;

    void appendChild(TreeModelItem *child);
    void appendChildren(const QVector<TreeModelItem*> & children);
    void removeChild(int row);
    void removeChildren(int first, int last);

protected:
    void emitDataChange();

private:
    struct Private;
    Private *const d;
};

class KCALLPRIVATE_EXPORT TreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    TreeModel(QObject *parent = 0);
    virtual ~TreeModel();

    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex & index) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

protected:
    inline TreeModelItem *root() const { return m_root; }

private:
    friend class TreeModelItem;
    TreeModelItem *m_root;
};

#endif
