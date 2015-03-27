/*
 * Copyright (C) 2014 Ekaitz Zárraga <ekaitz.zarraga@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DTMF_QML_H
#define DTMF_QML_H

#include <QMainWindow>
#include <QWidget>

#include <TelepathyQt/Constants>

//! Manages the GUI of the dialpad. \a Ekaitz
/*! \sa DtmfHandler, CallWindow::toggleDtmf()
 */

class DtmfQml : public QMainWindow
{
    Q_OBJECT

public:
    explicit DtmfQml(QWidget *parent = 0);
    virtual ~DtmfQml();

Q_SIGNALS:
    void startSendDtmfEvent(Tp::DTMFEvent event);
    void stopSendDtmfEvent();

private Q_SLOTS:
    void onButtonPressed(const QString &digit);

private:
    struct Private;
    Private *const d;
};

#endif //DTMF_QML_H
