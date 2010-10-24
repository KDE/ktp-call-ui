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
#ifndef CONTACTLISTCONTROLLER_H
#define CONTACTLISTCONTROLLER_H

#include <QtCore/QObject>
class QAbstractItemView;
class QAbstractItemModel;
class QPoint;

class ContactListController : public QObject
{
    Q_OBJECT
public:
    ContactListController(QAbstractItemView *view, QAbstractItemModel *model);
    virtual ~ContactListController();

private slots:
    void contextMenuRequested(const QPoint & pos);
    void callContactVoice();
    void callContactVideo();
    void callContact(bool useInitialVideo);
    void setStatus(int statusIndex);

private:
    struct Private;
    Private *const d;
};

#endif
