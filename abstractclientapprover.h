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
#ifndef ABSTRACTCLIENTAPPROVER_H
#define ABSTRACTCLIENTAPPROVER_H

#include <QtCore/QObject>
#include <TelepathyQt4/AbstractClientApprover>
namespace Tp { class PendingOperation; class DBusProxy; }

class ApproverRequest : public QObject
{
    Q_OBJECT
public:
    ApproverRequest(const Tp::MethodInvocationContextPtr<> & context,
                    const QList<Tp::ChannelPtr> & channels,
                    const Tp::ChannelDispatchOperationPtr & dispatchOperation);
    virtual ~ApproverRequest();

    Tp::MethodInvocationContextPtr<> context() const;
    QList<Tp::ChannelPtr> channels() const;
    Tp::ChannelDispatchOperationPtr dispatchOperation() const;

public slots:
    void accept();
    void reject();

signals:
    void ready(ApproverRequest *self);
    void finished(ApproverRequest *self);

private slots:
    void onDispatchOperationReady(Tp::PendingOperation *op);
    void onDispatchOperationInvalidated(Tp::DBusProxy *proxy,
                                        const QString & errorName,
                                        const QString & errorMessage);
    void onClaimFinished(Tp::PendingOperation *op);

private:
    struct Private;
    Private *const d;
};

class AbstractClientApprover : public QObject, public Tp::AbstractClientApprover
{
    Q_OBJECT
public:
    AbstractClientApprover(const Tp::ChannelClassList & classList);

protected slots:
    virtual void newRequest(ApproverRequest *request) = 0;
    virtual void requestFinished(ApproverRequest *request) = 0;

protected:
    virtual void addDispatchOperation(const Tp::MethodInvocationContextPtr<> & context,
                                      const QList<Tp::ChannelPtr> & channels,
                                      const Tp::ChannelDispatchOperationPtr & dispatchOperation);

private slots:
    void onRequestReady(ApproverRequest *request);
};

#endif
