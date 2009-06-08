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
#include "pendingoutgoingcall.h"
#include <KDebug>
#include <KLocalizedString>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingChannel>

struct PendingOutgoingCall::Private
{
    Tp::ContactPtr contact;
    Tp::ConnectionPtr connection;
    Tp::StreamedMediaChannelPtr channel;
};

PendingOutgoingCall::PendingOutgoingCall(Tp::ContactPtr contact, QObject *parent)
    : Tp::PendingOperation(parent), d(new Private)
{
    d->contact = contact;
    d->connection = contact->manager()->connection();
    connect(d->connection->becomeReady(/*Connection::FeatureSelfContact*/),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onOutgoingConnectionReady(Tp::PendingOperation*)));
}

PendingOutgoingCall::~PendingOutgoingCall()
{
    delete d;
}

Tp::StreamedMediaChannelPtr PendingOutgoingCall::channel() const
{
    return d->channel;
}

void PendingOutgoingCall::onOutgoingConnectionReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "Connection failed to become ready:" << op->errorName() << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                    QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                    Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                    d->contact->handle()[0]);

    Tp::PendingChannel *pendChan = d->connection->ensureChannel(request);
    connect(pendChan, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onCallRequestFinished(Tp::PendingOperation*)));
}

void PendingOutgoingCall::onCallRequestFinished(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "Failed to call contact:" << op->errorName() << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    Tp::PendingChannel *pendChan = qobject_cast<Tp::PendingChannel*>(op);
    Q_ASSERT(pendChan);

    if ( pendChan->yours() ) {
        d->channel = Tp::StreamedMediaChannel::create(pendChan->connection(),
                                                      pendChan->objectPath(),
                                                      pendChan->immutableProperties());

        connect(d->channel->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onOutgoingChannelReady(Tp::PendingOperation*)));
    } else {
        kDebug() << "Channel is not ours";
        setFinishedWithError(QLatin1String("Channel not ours"),
                             i18n("There is already an ongoing call with this contact"));
    }
}

void PendingOutgoingCall::onOutgoingChannelReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "Outgoing channel failed to become ready:" << op->errorName() << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    setFinished();
}

#include "pendingoutgoingcall.moc"
