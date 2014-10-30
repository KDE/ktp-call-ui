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
#include "../version.h"

#include <K4AboutData>
#include <KCmdLineArgs>
#include <KLocalizedString>
#include <KUniqueApplication>

#include <TelepathyQt/Types>

int main(int argc, char **argv)
{
    K4AboutData aboutData("ktp-dialout-ui", "ktp-call-ui", ki18n("KDE Telepathy Call Ui"),
                          KTP_CALL_UI_VERSION,
                          ki18n("VoIP client for KDE"), K4AboutData::License_GPL,
                          ki18n("(C) 2009-2012, George Kiagiadakis\n"
                                "(C) 2010-2011, Collabora Ltd."));
    aboutData.setProgramIconName("internet-telephony");
    aboutData.addAuthor(ki18nc("@info:credit", "George Kiagiadakis"), KLocalizedString(),
                         "kiagiadakis.george@gmail.com");
    aboutData.setProductName("telepathy/call-ui"); //set the correct name for bug reporting


    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("+[number]", ki18n("The number to call"));

    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KUniqueApplication app;

    Tp::registerTypes();

    MainWindow *mw;
    if(args->count()) {
        mw = new MainWindow(args->arg(0));
    } else {
        mw = new MainWindow();
    }
    mw->show();

    return app.exec();
}
