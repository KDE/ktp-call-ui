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

#include "dtmf-qml.h"

#include <QGraphicsObject>
#include <QQuickView>
#include <QQuickItem>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KDeclarative/KDeclarative>

struct DtmfQml::Private
{
    QQuickView *view;
    QWidget *viewContainer;
    KDeclarative::KDeclarative kd;
};

DtmfQml::DtmfQml(QWidget *parent)
    : QMainWindow(parent), d(new Private)
{
    d->view = new QQuickView();
    d->kd.setDeclarativeEngine(d->view->engine());
    d->kd.setupBindings();
    d->view->setSource(QUrl(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("ktp-call-ui/core/Dtmf.qml"))));
    d->view->setResizeMode(QQuickView::SizeRootObjectToView);
    d->viewContainer = QWidget::createWindowContainer(d->view, this);
    setFixedSize(d->view->size());
    setWindowTitle( i18n("Dialpad"));
    setCentralWidget(d->viewContainer);
    connect(d->view->rootObject(), SIGNAL(release(QString)), this, SIGNAL(stopSendDtmfEvent()));
    connect(d->view->rootObject(), SIGNAL(press(QString)), this, SLOT(onButtonPressed(QString)));
}

DtmfQml::~DtmfQml()
{
    delete d;
}

void DtmfQml::onButtonPressed(const QString &button)
{
    Tp::DTMFEvent code;

    if (1 != button.length()) {
        return;
    }

    if (QChar('#') == button.at(0)) {
        code = Tp::DTMFEventHash;
    } else if (QChar('*') == button.at(0)) {
        code = Tp::DTMFEventAsterisk;
    } else if (QChar('A') == button.at(0)) {
        code = Tp::DTMFEventLetterA;
    } else if (QChar('B') == button.at(0)) {
        code = Tp::DTMFEventLetterB;
    } else if (QChar('C') == button.at(0)) {
        code = Tp::DTMFEventLetterC;
    } else if (QChar('D') == button.at(0)) {
        code = Tp::DTMFEventLetterD;
    } else {
        // should we catch if button is not a digit?
        code = static_cast<Tp::DTMFEvent>(button.toInt());
    }
    Q_EMIT startSendDtmfEvent(code);
}
