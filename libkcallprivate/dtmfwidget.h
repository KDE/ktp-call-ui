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
#ifndef DTMFWIDGET_H
#define DTMFWIDGET_H

#include "kcallprivate_export.h"
#include <QtGui/QWidget>
#include <TelepathyQt4/Constants>

class KCALLPRIVATE_EXPORT DtmfWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DtmfWidget(QWidget *parent = 0);
    virtual ~DtmfWidget();

Q_SIGNALS:
    void startSendDtmfEvent(Tp::DTMFEvent event);
    void stopSendDtmfEvent();

private Q_SLOTS:
    void onButtonPressed(int code);

private:
    struct Private;
    Private *const d;
};

#endif
