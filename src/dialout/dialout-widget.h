/*
    Copyright (C) 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DIALOUT_WIDGET_H
#define DIALOUT_WIDGET_H

#include <QWidget>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>

class DialoutWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DialoutWidget(const QString &number = QString(), QWidget *parent = 0);
    virtual ~DialoutWidget();

private Q_SLOTS:
    void onAccountManagerReady(Tp::PendingOperation *op);
    void onRowsChanged();

    void on_accountComboBox_currentIndexChanged(int currentIndex);
    void on_uriLineEdit_textChanged(const QString &text);
    void onPendingContactFinished(Tp::PendingOperation*);

    void on_audioCallButton_clicked();
    void on_videoCallButton_clicked();
    void onPendingChannelRequestFinished(Tp::PendingOperation*);

private:
    void requestContact(const Tp::AccountPtr &account, const QString &contactId);

    struct Private;
    Private *const d;
};

#endif // DIALOUT_WIDGET_H
