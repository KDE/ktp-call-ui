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
#ifndef ABSTRACTCLIENTAPPROVER_H
#define ABSTRACTCLIENTAPPROVER_H

#include "kcallprivate_export.h"
#include <QtCore/QObject>
#include <TelepathyQt4/AbstractClientApprover>
namespace Tp { class PendingOperation; class DBusProxy; }

/** This class represents an approval request. AbstractClientApprover is responsible
 * for constructing instances of this class and passing them to your approver subclass through
 * AbstractClientApprover::newRequest().
 * @todo Improve this class so that it can be used outside the scope of KCall.
 */
class KCALLPRIVATE_EXPORT ApproverRequest : public QObject
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

/** This is an extension to Tp::AbstractClientApprover that offers a higher level API
 * for handling approval requests. To use it, you have to subclass it and implement
 * the pure virtual slots newRequest() and requestFinished().
 */
class KCALLPRIVATE_EXPORT AbstractClientApprover : public QObject, public Tp::AbstractClientApprover
{
    Q_OBJECT
public:
    /** Constructs a new Approver that can handle the channel types listed in @a classList.
     * For more information on what @a classList should contain, refer to the telepathy spec.
     */
    AbstractClientApprover(const Tp::ChannelClassList & classList);

protected slots:
    /** This method is called when a new approval request arrives. In the implementation
     * of this method you should notify the user about the new request.
     */
    virtual void newRequest(ApproverRequest *request) = 0;

    /** This method is called when a request has finished. This can happen either because
     * ApproverRequest::accept() or ApproverRequest::reject() was called on the request
     * or because some other approver has already handled this request.
     */
    virtual void requestFinished(ApproverRequest *request) = 0;

protected:
    virtual void addDispatchOperation(const Tp::MethodInvocationContextPtr<> & context,
                                      const QList<Tp::ChannelPtr> & channels,
                                      const Tp::ChannelDispatchOperationPtr & dispatchOperation);

private slots:
    void onRequestReady(ApproverRequest *request);
};

#endif
