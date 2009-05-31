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
#ifndef CALLWINDOW_H
#define CALLWINDOW_H

#include <KXmlGuiWindow>
#include <TelepathyQt4/Contact>

class CallWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    CallWindow();
    virtual ~CallWindow();

    void callContact(Tp::ContactPtr contact);

private slots:
    void callRequestFinished(Tp::PendingOperation *op);

private:
    struct Private;
    Private *const d;
};

#endif
