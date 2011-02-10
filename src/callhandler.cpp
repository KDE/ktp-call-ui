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
#include <TelepathyQt4Yell/CallChannel>
#include <TelepathyQt4Yell/ChannelClassSpec>

static inline Tp::ChannelClassSpecList channelClassSpecList()
{
    return Tp::ChannelClassSpecList() << Tpy::ChannelClassSpec::audioCall()
                                      << Tpy::ChannelClassSpec::videoCall()
                                      << Tpy::ChannelClassSpec::videoCallWithAudio();
}

CallHandler::CallHandler()
    : Tp::AbstractClientHandler(channelClassSpecList())
{
    kDebug();
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
                                 const Tp::AbstractClientHandler::HandlerInfo & handlerInfo)
{
    kDebug();
    Q_UNUSED(account);
    Q_UNUSED(connection);
    Q_UNUSED(requestsSatisfied);
    Q_UNUSED(userActionTime);
    Q_UNUSED(handlerInfo);

    Q_FOREACH(const Tp::ChannelPtr & channel, channels) {
        Tpy::CallChannelPtr callChannel = Tpy::CallChannelPtr::dynamicCast(channel);
        if (!callChannel) {
            kDebug() << "Channel is not a Call channel. Ignoring";
            continue;
        }

        callChannel->accept();

        CallWindow *cw = new CallWindow(callChannel);
        cw->show();
    }

    context->setFinished();
}

#include "callhandler.moc"
