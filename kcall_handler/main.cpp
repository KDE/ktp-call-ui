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
#include "callhandler.h"

#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocalizedString>
#include <KUniqueApplication>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/ClientRegistrar>

int main(int argc, char **argv)
{
    KAboutData aboutData("kcall_handler", "kcall", ki18n("KCall"), "0.8",
                          ki18n("VoIP client for KDE"), KAboutData::License_GPL,
                          ki18n("(C) 2009, George Kiagiadakis"));
    aboutData.setProgramIconName("internet-telephony");
    aboutData.addAuthor(ki18nc("@info:credit", "George Kiagiadakis"), KLocalizedString(),
                         "kiagiadakis.george@gmail.com");

    KCmdLineArgs::init(argc, argv, &aboutData);

    if ( !KUniqueApplication::start() ) {
        return 0;
    }

    KUniqueApplication app;
    app.disableSessionManagement();

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    Tp::ClientRegistrarPtr registrar = Tp::ClientRegistrar::create();
    Tp::SharedPtr<CallHandler> callHandler = Tp::SharedPtr<CallHandler>(new CallHandler());
    registrar->registerClient(Tp::AbstractClientPtr::dynamicCast(callHandler), "kcall_handler");

    return app.exec();
}
