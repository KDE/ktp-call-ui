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
#include "callhandler.h"
#include "callwindow.h"
#include <KDebug>
#include <TelepathyQt4/StreamedMediaChannel>

static inline Tp::ChannelClassList channelClassList()
{
    QMap<QString, QDBusVariant> class1;
    class1[TELEPATHY_INTERFACE_CHANNEL ".ChannelType"] =
                                    QDBusVariant(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    class1[TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"] = QDBusVariant(Tp::HandleTypeContact);

    QMap<QString, QDBusVariant> class2;
    class2[TELEPATHY_INTERFACE_CHANNEL ".ChannelType"] =
                                    QDBusVariant(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    class2[TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"] = QDBusVariant(Tp::HandleTypeContact);
    class2[TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".FUTURE.InitialAudio"] = QDBusVariant(true);

    return Tp::ChannelClassList() << Tp::ChannelClass(class1) << Tp::ChannelClass(class2);
}

CallHandler::CallHandler()
    : Tp::AbstractClientHandler(channelClassList(), false)
{
}

CallHandler::~CallHandler()
{
}

bool CallHandler::bypassApproval() const
{
    return false;
}

void CallHandler::handleChannels(const Tp::MethodInvocationContextPtr<> & context,
                                 const Tp::AccountPtr & account,
                                 const Tp::ConnectionPtr & connection,
                                 const QList<Tp::ChannelPtr> & channels,
                                 const QList<Tp::ChannelRequestPtr> & requestsSatisfied,
                                 const QDateTime & userActionTime,
                                 const QVariantMap & handlerInfo)
{
    Q_UNUSED(account);
    Q_UNUSED(connection);
    Q_UNUSED(requestsSatisfied);
    Q_UNUSED(userActionTime);
    Q_UNUSED(handlerInfo);

    foreach(const Tp::ChannelPtr & channel, channels) {
        Tp::StreamedMediaChannel *smchannel = qobject_cast<Tp::StreamedMediaChannel*>(channel.data());
        if ( !smchannel ) {
            kDebug() << "Channel is not a streamed media channel. Ignoring";
            continue;
        }

        kDebug() << "handling new channel";
        CallWindow *cw = new CallWindow(Tp::StreamedMediaChannelPtr(smchannel));
        cw->show();
    }

    context->setFinished();
}
