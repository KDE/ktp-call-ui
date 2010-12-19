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
#include "abstractclientapprover.h"
#include <KDebug>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ChannelDispatchOperation>
#include <TelepathyQt4/Channel>

struct ApproverRequest::Private
{
    Tp::MethodInvocationContextPtr<> context;
    QList<Tp::ChannelPtr> channels;
    Tp::ChannelDispatchOperationPtr dispatchOperation;
};

ApproverRequest::ApproverRequest(const Tp::MethodInvocationContextPtr<> & context,
                                 const QList<Tp::ChannelPtr> & channels,
                                 const Tp::ChannelDispatchOperationPtr & dispatchOperation)
    : QObject(), d(new Private)
{
    d->context = context;
    d->channels = channels;
    d->dispatchOperation = dispatchOperation;

    connect(dispatchOperation->becomeReady(), SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onDispatchOperationReady(Tp::PendingOperation*)));
    connect(this, SIGNAL(finished(ApproverRequest*)), SLOT(deleteLater()));
}

ApproverRequest::~ApproverRequest()
{
    delete d;
}

Tp::MethodInvocationContextPtr<> ApproverRequest::context() const
{
    return d->context;
}

QList<Tp::ChannelPtr> ApproverRequest::channels() const
{
    return d->channels;
}

Tp::ChannelDispatchOperationPtr ApproverRequest::dispatchOperation() const
{
    return d->dispatchOperation;
}

void ApproverRequest::accept()
{
    d->dispatchOperation->handleWith("org.freedesktop.Telepathy.Client.kcall_handler");
    emit finished(this);
}

void ApproverRequest::reject()
{
    connect(d->dispatchOperation->claim(), SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onClaimFinished(Tp::PendingOperation*)));
}

void ApproverRequest::onDispatchOperationReady(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kError() << "Dispatch operation failed to become ready"
                 << op->errorName() << op->errorMessage();
        d->context->setFinishedWithError(op->errorName(), op->errorMessage()); //### what should the error message be?
        emit finished(this);
        return;
    }

    connect(d->dispatchOperation.data(), SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            SLOT(onDispatchOperationInvalidated(Tp::DBusProxy*, QString, QString)));
    emit ready(this);
    d->context->setFinished();
}

void ApproverRequest::onDispatchOperationInvalidated(Tp::DBusProxy *proxy,
                                                     const QString & errorName,
                                                     const QString & errorMessage)
{
    Q_UNUSED(proxy);
    if ( errorName != TELEPATHY_QT4_ERROR_OBJECT_REMOVED ) {
        kError() << errorName << errorMessage;
    }
    emit finished(this);
}

void ApproverRequest::onClaimFinished(Tp::PendingOperation *op)
{
    if ( !op->isError() ) {
        foreach(const Tp::ChannelPtr & channel, d->channels) {
            channel->requestClose();
        }
    }

    emit finished(this);
}

AbstractClientApprover::AbstractClientApprover(const Tp::ChannelClassList & classList)
    : QObject(), Tp::AbstractClientApprover(classList)
{
}

void AbstractClientApprover::addDispatchOperation(const Tp::MethodInvocationContextPtr<> & context,
                                        const QList<Tp::ChannelPtr> & channels,
                                        const Tp::ChannelDispatchOperationPtr & dispatchOperation)
{
    ApproverRequest *r = new ApproverRequest(context, channels, dispatchOperation);
    connect(r, SIGNAL(ready(ApproverRequest*)), SLOT(onRequestReady(ApproverRequest*)));
}

void AbstractClientApprover::onRequestReady(ApproverRequest *request)
{
    newRequest(request);
    connect(request, SIGNAL(finished(ApproverRequest*)), SLOT(requestFinished(ApproverRequest*)));
}


#include "abstractclientapprover.moc"
