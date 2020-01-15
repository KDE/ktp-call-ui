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

#include <KAboutData>
#include <KLocalizedString>

#include <QApplication>

#include <TelepathyQt/Types>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("ktp-call-ui");

    KAboutData aboutData("ktp-dialout-ui", i18n("KDE Telepathy Dialout Ui"),
                        KTP_CALL_UI_VERSION,
                        i18n("VoIP client by KDE"), KAboutLicense::GPL,
                        i18n("(C) 2009-2012, George Kiagiadakis\n"
                             "(C) 2010-2011, Collabora Ltd."));
    aboutData.addAuthor(i18nc("@info:credit", "George Kiagiadakis"), QString(),
                         "kiagiadakis.george@gmail.com");
    aboutData.setProductName("telepathy/call-ui"); //set the correct name for bug reporting
    KAboutData::setApplicationData(aboutData);

    app.setWindowIcon(QIcon::fromTheme("internet-telephony"));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    parser.addPositionalArgument("+[number]", i18n("The number to call"));

    parser.process(app);
    aboutData.processCommandLine(&parser);
    const QStringList args = parser.positionalArguments();

    Tp::registerTypes();

    MainWindow *mw;
    if (args.count()) {
        mw = new MainWindow(args[0]);
    } else {
        mw = new MainWindow();
    }
    mw->show();

    return app.exec();
}
