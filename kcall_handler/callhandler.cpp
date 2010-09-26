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
#include "callwindow.h"
#include <KDebug>
#include <TelepathyQt4/StreamedMediaChannel>

static inline Tp::ChannelClassList channelClassList()
{
    QMap<QString, QDBusVariant> class1;
    class1[TELEPATHY_INTERFACE_CHANNEL ".ChannelType"] =
                                    QDBusVariant(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    class1[TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"] = QDBusVariant(Tp::HandleTypeContact);

    return Tp::ChannelClassList() << Tp::ChannelClass(class1);
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
        Tp::StreamedMediaChannelPtr smchannel = Tp::StreamedMediaChannelPtr::dynamicCast(channel);
        if ( !smchannel ) {
            kDebug() << "Channel is not a streamed media channel. Ignoring";
            continue;
        }

        kDebug() << "handling new channel";
        connect(smchannel->becomeReady(Tp::Features()
                                        << Tp::StreamedMediaChannel::FeatureCore
                                        << Tp::StreamedMediaChannel::FeatureStreams),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onChannelReady(Tp::PendingOperation*)));
    }

    context->setFinished();
}

void CallHandler::onChannelReady(Tp::PendingOperation *operation)
{
    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(operation);
    if (pr->isError()) {
        kError() << "Channel failed to become ready";
        return;
    }

    Tp::StreamedMediaChannelPtr channel(qobject_cast<Tp::StreamedMediaChannel*>(pr->object()));
    CallWindow *cw = new CallWindow(channel);
    cw->show();
}

#include "callhandler.moc"
