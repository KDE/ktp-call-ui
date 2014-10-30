/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
    Copyright (C) 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "sink-manager.h"
#include "tf-content-handler.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/ReferencedHandles>

#include <QGlib/Connect>
#include <QGst/ElementFactory>
#include <QGst/Pipeline>
#include <QGst/Pad>

#include <KDebug>

namespace KTpCallPrivate {

SinkManager::SinkManager(TfContentHandler *parent)
    : QObject(parent),
      m_contactManager(parent->channelHandler()->callChannel()->connection()->contactManager())
{
}

SinkManager::~SinkManager()
{
}

void SinkManager::cleanup()
{
    QMutexLocker l(&m_mutex);

    kDebug() << m_controllers.size() << "pads need unlinking";

    /* unlink all pads */
    QHashIterator<QGst::PadPtr, BaseSinkController*> it(m_controllers);
    while (it.hasNext()) {
        it.next();

        QGst::PadPtr srcPad = it.key();
        QGst::PadPtr sinkPad = srcPad->peer();

        QGlib::disconnect(srcPad, 0, this, 0);
        srcPad->unlink(sinkPad);

        Q_ASSERT(!m_releasedControllers.contains(it.value()));

        contentHandler()->releaseSinkControllerData(it.value());
        Q_EMIT controllerDestroyed(it.value());
        delete it.value();
    }
    m_controllers.clear();

    //this is not empty when onPadUnlinked() was called from the streaming thread
    //but destroyController() has not yet been executed in the main thread.
    //since this is the destructor, we need to cleanup because destroyController()
    //will never be actually executed.
    Q_FOREACH(BaseSinkController *ctrl, m_releasedControllers) {
        Q_EMIT controllerDestroyed(ctrl);
        delete ctrl;
    }
    m_releasedControllers.clear();
}

void SinkManager::handleNewSinkPad(uint contactHandle, const QGst::PadPtr & pad)
{
    kDebug() << "New src pad" << pad->name() << "from handle" << contactHandle;

    //link the pad
    BaseSinkController *ctrl = contentHandler()->createSinkController(pad);

    //notify when the pad is unlinked (probably because it was removed from the fsconference)
    QGlib::connect(pad, "unlinked", this, &SinkManager::onPadUnlinked, QGlib::PassSender);

    m_mutex.lock();
    m_controllersWaitingForContact.insert(contactHandle, ctrl);
    m_controllers.insert(pad, ctrl);
    m_mutex.unlock();

    //continue processing from the main thread
    QMetaObject::invokeMethod(this, "handleNewSinkPadAsync", Qt::QueuedConnection,
                              Q_ARG(uint, contactHandle));
}

void SinkManager::handleNewSinkPadAsync(uint contactHandle)
{
    QMutexLocker l(&m_mutex);

    BaseSinkController *ctrl = m_controllersWaitingForContact.value(contactHandle);
    if (Q_UNLIKELY(!ctrl)) {
        //just in case the pad was unlinked before we reached this slot
        kDebug() << "Not handling new pad. The pad was unlinked too early.";
        return;
    }

    Tp::PendingContacts *pc = m_contactManager->contactsForHandles(
                Tp::UIntList() << contactHandle, Tp::Features());
    connect(pc, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onPendingContactsFinished(Tp::PendingOperation*)));
    pc->setProperty("ktpcall_contact_handle", QVariant::fromValue(contactHandle));
}

void SinkManager::onPendingContactsFinished(Tp::PendingOperation *op)
{
    QMutexLocker l(&m_mutex);

    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts*>(op);
    Q_ASSERT(pc);

    uint contactHandle = pc->property("ktpcall_contact_handle").toUInt();
    BaseSinkController *ctrl = m_controllersWaitingForContact.value(contactHandle);
    if (Q_UNLIKELY(!ctrl)) {
        // just in case the pad was unlinked before we reached this slot
        kDebug() << "Not handling new pad. The pad was unlinked too early.";
        return;
    }

    // this can't really fail because the contact is already known
    if (Q_UNLIKELY(pc->isError())) {
        kError() << "Failed to fetch contact for handle" << contactHandle
                 << op->errorName() << op->errorMessage();
        m_controllersWaitingForContact.remove(contactHandle);
        return;
    }

    QList<Tp::ContactPtr> contacts = pc->contacts();
    Q_ASSERT(contacts.size() == 1);

    m_controllersWaitingForContact.remove(contactHandle);
    ctrl->initFromMainThread(contacts.at(0));
    Q_EMIT controllerCreated(ctrl);
}

void SinkManager::onPadUnlinked(const QGst::PadPtr & srcPad)
{
    QMutexLocker l(&m_mutex);

    kDebug() << srcPad->name() << "was unlinked.";
    kDebug() << "Current thread:" << QThread::currentThread()
             << "Main thread:" << QCoreApplication::instance()->thread();

    BaseSinkController *ctrl = m_controllers.value(srcPad);

    if (Q_UNLIKELY(!ctrl)) {
        //just in case unlinkAllPads() locked the mutex first...
        kDebug() << "Looks like unlinkAllPads() wins...";
        return;
    }

    //check if the controller still has not finished its initialization
    QMutableHashIterator<uint, BaseSinkController*> it(m_controllersWaitingForContact);
    while (it.hasNext()) {
        it.next();
        if (it.value() == ctrl) {
            it.remove();
        }
    }

    //release data now
    contentHandler()->releaseSinkControllerData(ctrl);

    //delete from the main thread, if we are not being called from the main thread
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        m_releasedControllers.insert(ctrl);
        QMetaObject::invokeMethod(this, "destroyController", Qt::QueuedConnection,
                                  Q_ARG(BaseSinkController*, ctrl));
    } else {
        Q_EMIT controllerDestroyed(ctrl);
        delete ctrl;
    }

    m_controllers.remove(srcPad);
}

void SinkManager::destroyController(BaseSinkController *controller)
{
    Q_EMIT controllerDestroyed(controller);
    delete controller;

    m_mutex.lock();
    m_releasedControllers.remove(controller);
    m_mutex.unlock();
}

} // KTpCallPrivate
