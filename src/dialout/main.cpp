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

#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocalizedString>
#include <KUniqueApplication>

#include <TelepathyQt/Types>

int main(int argc, char **argv)
{
    KAboutData aboutData("ktp-dialout-ui", "ktp-call-ui", ki18n("KDE Telepathy Call Ui"), "0.4",
                          ki18n("VoIP client for KDE"), KAboutData::License_GPL,
                          ki18n("(C) 2009-2012, George Kiagiadakis\n"
                                "(C) 2010-2011, Collabora Ltd."));
    aboutData.setProgramIconName("internet-telephony");
    aboutData.addAuthor(ki18nc("@info:credit", "George Kiagiadakis"), KLocalizedString(),
                         "kiagiadakis.george@gmail.com");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KUniqueApplication app;

    Tp::registerTypes();

    MainWindow *mw = new MainWindow;
    mw->show();

    return app.exec();
}
