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

#include "main-window.h"
#include "dialout-widget.h"

#include <KLocalizedString>

MainWindow::MainWindow(const QString &number, QWidget *parent, Qt::WindowFlags f)
    : KXmlGuiWindow(parent, f)
{
    setCentralWidget(new DialoutWidget(number, this));
    setupGUI(QSize(450, 120), KXmlGuiWindow::Create | KXmlGuiWindow::Save);
    setWindowTitle(i18n("Make a call"));
}

MainWindow::~MainWindow()
{
}
