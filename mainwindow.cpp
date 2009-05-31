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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "kcallapplication.h"
#include "contactlistcontroller.h"
#include "models/contactsmodel.h"
#include <KStatusBar>
#include <KAction>
#include <KActionCollection>

MainWindow::MainWindow()
    : KXmlGuiWindow(),
      ui(new Ui::MainWindow)
{
    setWindowTitle(i18nc("@title:window", "KCall"));

    QWidget *centralWidget = new QWidget(this);
    ui->setupUi(centralWidget);
    setCentralWidget(centralWidget);

    ui->contactsTreeView->setModel(KCallApplication::instance()->contactsModel());
    new ContactListController(ui->contactsTreeView, KCallApplication::instance()->contactsModel());

    setupActions();
    setupGUI(QSize(340, 460));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupActions()
{
    KStandardAction::quit(KCallApplication::instance(), SLOT(quit()), actionCollection());
}

#include "mainwindow.moc"
