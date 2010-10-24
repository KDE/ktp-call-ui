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
#ifndef SYSTRAYICON_H
#define SYSTRAYICON_H

#include "libkcallprivate/abstractclientapprover.h"

class SystrayIcon : public AbstractClientApprover
{
    Q_OBJECT
public:
    SystrayIcon();
    virtual ~SystrayIcon();

protected slots:
    virtual void newRequest(ApproverRequest *request);
    virtual void requestFinished(ApproverRequest *request);

private slots:
    void onActivateRequested(bool active, const QPoint & pos);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    struct Private;
    Private *const d;
};

#endif
