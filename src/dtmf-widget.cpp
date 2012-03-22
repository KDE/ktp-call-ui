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
#include "dtmf-widget.h"
#include "ui_dtmf-widget.h"
#include <QtCore/QSignalMapper>

struct DtmfWidget::Private
{
    Ui::DtmfWidget ui;
    QSignalMapper mapper;
};

DtmfWidget::DtmfWidget(QWidget *parent)
    : QWidget(parent), d(new Private)
{
    d->ui.setupUi(this);
    connect(&d->mapper, SIGNAL(mapped(int)), SLOT(onButtonPressed(int)));

    d->mapper.setMapping(d->ui.button0, Tp::DTMFEventDigit0);
    d->mapper.setMapping(d->ui.button1, Tp::DTMFEventDigit1);
    d->mapper.setMapping(d->ui.button2, Tp::DTMFEventDigit2);
    d->mapper.setMapping(d->ui.button3, Tp::DTMFEventDigit3);
    d->mapper.setMapping(d->ui.button4, Tp::DTMFEventDigit4);
    d->mapper.setMapping(d->ui.button5, Tp::DTMFEventDigit5);
    d->mapper.setMapping(d->ui.button6, Tp::DTMFEventDigit6);
    d->mapper.setMapping(d->ui.button7, Tp::DTMFEventDigit7);
    d->mapper.setMapping(d->ui.button8, Tp::DTMFEventDigit8);
    d->mapper.setMapping(d->ui.button9, Tp::DTMFEventDigit9);
    d->mapper.setMapping(d->ui.buttonAsterisk, Tp::DTMFEventAsterisk);
    d->mapper.setMapping(d->ui.buttonHash, Tp::DTMFEventHash);

    connect(d->ui.button0, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button1, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button2, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button3, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button4, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button5, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button6, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button7, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button8, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.button9, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.buttonAsterisk, SIGNAL(pressed()), &d->mapper, SLOT(map()));
    connect(d->ui.buttonHash, SIGNAL(pressed()), &d->mapper, SLOT(map()));

    connect(d->ui.button0, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button1, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button2, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button3, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button4, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button5, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button6, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button7, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button8, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.button9, SIGNAL(released()),  SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.buttonAsterisk, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
    connect(d->ui.buttonHash, SIGNAL(released()), SIGNAL(stopSendDtmfEvent()));
}

DtmfWidget::~DtmfWidget()
{
    delete d;
}

void DtmfWidget::onButtonPressed(int code)
{
    Q_EMIT startSendDtmfEvent(static_cast<Tp::DTMFEvent>(code));
}
