/*
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>
    Copyright (C) 2011 Collabora Ltd.
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

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

#include <TelepathyQt/Types>
#include <TelepathyQt/Debug>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/CallChannel>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelFactory>

int main(int argc, char **argv)
{
    KAboutData aboutData("ktp-call-ui", 0, ki18n("KDE Telepathy Call Ui"), "0.3.60",
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

    Tp::AccountFactoryPtr accountFactory = Tp::AccountFactory::create(
        QDBusConnection::sessionBus(),
        Tp::Features() << Tp::Account::FeatureCore
    );

    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
        QDBusConnection::sessionBus(),
        Tp::Features() << Tp::Connection::FeatureCore
                       << Tp::Connection::FeatureSelfContact
    );

    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(
        QDBusConnection::sessionBus()
    );
    channelFactory->addCommonFeatures(Tp::Channel::FeatureCore);
    channelFactory->addFeaturesForCalls(
        Tp::Features() << Tp::CallChannel::FeatureContents
                       << Tp::CallChannel::FeatureCallState
    );

    Tp::ContactFactoryPtr contactFactory = Tp::ContactFactory::create(
        Tp::Features() << Tp::Contact::FeatureAlias
                       << Tp::Contact::FeatureAvatarData
    );

    Tp::ClientRegistrarPtr registrar =
        Tp::ClientRegistrar::create(accountFactory, connectionFactory,
                                    channelFactory, contactFactory);

    Tp::SharedPtr<CallHandler> callHandler = Tp::SharedPtr<CallHandler>(new CallHandler());
    registrar->registerClient(Tp::AbstractClientPtr::dynamicCast(callHandler), "KTp.CallUi");

    return app.exec();
}
