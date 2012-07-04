/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

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
#include "sink-managers.h"
#include "device-element-factory.h"

#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/ReferencedHandles>

#include <QGlib/Connect>
#include <QGst/ElementFactory>
#include <QGst/Pipeline>
#include <QGst/Pad>

#include <KDebug>

//BEGIN BaseSinkManager

BaseSinkManager::BaseSinkManager(const Tp::ContactManagerPtr & contactManager, QObject *parent)
    : QObject(parent),
      m_contactManager(contactManager)
{
}

BaseSinkManager::~BaseSinkManager()
{
}

void BaseSinkManager::handleNewSinkPad(uint contactHandle, const QGst::PadPtr & pad)
{
    kDebug() << "New src pad" << pad->name() << "from handle" << contactHandle;

    //link the pad
    BaseSinkController *ctrl = createController(pad);

    //notify when the pad is unlinked (probably because it was removed from the fsconference)
    QGlib::connect(pad, "unlinked", this, &BaseSinkManager::onPadUnlinked, QGlib::PassSender);

    m_mutex.lock();
    m_controllersWaitingForContact.insert(contactHandle, ctrl);
    m_controllers.insert(pad, ctrl);
    m_mutex.unlock();

    //continue processing from the main thread
    QMetaObject::invokeMethod(this, "handleNewSinkPadAsync", Qt::QueuedConnection,
                              Q_ARG(uint, contactHandle));
}

void BaseSinkManager::handleNewSinkPadAsync(uint contactHandle)
{
    QMutexLocker l(&m_mutex);

    BaseSinkController *ctrl = m_controllersWaitingForContact.value(contactHandle);
    if (KDE_ISUNLIKELY(!ctrl)) {
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

void BaseSinkManager::onPendingContactsFinished(Tp::PendingOperation *op)
{
    QMutexLocker l(&m_mutex);

    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts*>(op);
    Q_ASSERT(pc);

    uint contactHandle = pc->property("ktpcall_contact_handle").toUInt();
    BaseSinkController *ctrl = m_controllersWaitingForContact.value(contactHandle);
    if (KDE_ISUNLIKELY(!ctrl)) {
        // just in case the pad was unlinked before we reached this slot
        kDebug() << "Not handling new pad. The pad was unlinked too early.";
        return;
    }

    // this can't really fail because the contact is already known
    if (KDE_ISUNLIKELY(pc->isError())) {
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

void BaseSinkManager::unlinkAllPads()
{
    QMutexLocker l(&m_mutex);

    kDebug() << m_controllers.size() << "pads need unlinking";

    QHashIterator<QGst::PadPtr, BaseSinkController*> it(m_controllers);
    while (it.hasNext()) {
        it.next();

        QGst::PadPtr srcPad = it.key();
        QGst::PadPtr sinkPad = srcPad->peer();

        QGlib::disconnect(srcPad, 0, this, 0);
        srcPad->unlink(sinkPad);

        Q_ASSERT(!m_releasedControllers.contains(it.value()));

        releaseControllerData(it.value());
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

void BaseSinkManager::onPadUnlinked(const QGst::PadPtr & srcPad)
{
    QMutexLocker l(&m_mutex);

    kDebug() << srcPad->name() << "was unlinked.";
    kDebug() << "Current thread:" << QThread::currentThread()
             << "Main thread:" << QCoreApplication::instance()->thread();

    BaseSinkController *ctrl = m_controllers.value(srcPad);

    if (KDE_ISUNLIKELY(!ctrl)) {
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
    releaseControllerData(ctrl);

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

void BaseSinkManager::destroyController(BaseSinkController *controller)
{
    Q_EMIT controllerDestroyed(controller);
    delete controller;

    m_mutex.lock();
    m_releasedControllers.remove(controller);
    m_mutex.unlock();
}

//END BaseSinkManager
//BEGIN AudioSinkManager

AudioSinkManager::AudioSinkManager(const QGst::PipelinePtr & pipeline,
            const Tp::ContactManagerPtr & contactManager, QObject *parent)
    : BaseSinkManager(contactManager, parent),
      m_pipeline(pipeline),
      m_sinkRefCount(0)
{
}

AudioSinkManager::~AudioSinkManager()
{
    Q_ASSERT(m_sinkRefCount == 0);
}

BaseSinkController *AudioSinkManager::createController(const QGst::PadPtr & srcPad)
{
    refSink();
    AudioSinkController *ctrl = new AudioSinkController(m_adder->getRequestPad("sink%d"));
    ctrl->initFromStreamingThread(srcPad, m_pipeline);
    return ctrl;
}

void AudioSinkManager::releaseControllerData(BaseSinkController *ctrl)
{
    AudioSinkController *actrl = static_cast<AudioSinkController*>(ctrl);
    QGst::PadPtr adderRequestPad = actrl->adderRequestPad();

    ctrl->releaseFromStreamingThread(m_pipeline);

    m_adder->releaseRequestPad(adderRequestPad);
    adderRequestPad.clear();

    unrefSink();
}

void AudioSinkManager::refSink()
{
    QMutexLocker l(&m_mutex);
    if (m_sinkRefCount++ == 0) {
        m_sink = DeviceElementFactory::makeAudioOutputElement();
        if (!m_sink) {
            kWarning() << "Failed to create audio sink. Using fakesink. "
                        "You will need to restart the call to get audio output.";
            m_sink = QGst::ElementFactory::make("fakesink");
            m_sink->setProperty("sync", false);
            m_sink->setProperty("async", false);
            m_sink->setProperty("silent", true);
            m_sink->setProperty("enable-last-buffer", false);
        }

        m_adder = QGst::ElementFactory::make("liveadder");
        if (!m_adder) {
            kWarning() << "Failed to create liveadder. Using adder. This may cause trouble...";
            m_adder = QGst::ElementFactory::make("adder");
        }

        m_pipeline->add(m_adder, m_sink);
        m_adder->link(m_sink);
        m_sink->syncStateWithParent();
        m_adder->syncStateWithParent();
    }
}

void AudioSinkManager::unrefSink()
{
    QMutexLocker l(&m_mutex);
    if (--m_sinkRefCount == 0) {
        m_adder->unlink(m_sink);
        m_adder->setState(QGst::StateNull);
        m_sink->setState(QGst::StateNull);
        m_pipeline->remove(m_adder);
        m_pipeline->remove(m_sink);
        m_adder.clear();
        m_sink.clear();
    }
}

//END AudioSinkManager
//BEGIN VideoSinkManager

VideoSinkManager::VideoSinkManager(const QGst::PipelinePtr & pipeline,
            const Tp::ContactManagerPtr & contactManager, QObject *parent)
    : BaseSinkManager(contactManager, parent),
      m_pipeline(pipeline)
{
}

BaseSinkController *VideoSinkManager::createController(const QGst::PadPtr & srcPad)
{
    VideoSinkController *ctrl = new VideoSinkController;
    ctrl->initFromStreamingThread(srcPad, m_pipeline);
    return ctrl;
}

void VideoSinkManager::releaseControllerData(BaseSinkController *ctrl)
{
    ctrl->releaseFromStreamingThread(m_pipeline);
}

//END VideoSinkManager

#include "moc_sink-managers.cpp"
