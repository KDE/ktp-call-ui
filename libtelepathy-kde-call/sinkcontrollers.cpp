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
#include "sinkcontrollers_p.h"
#include "deviceelementfactory_p.h"
#include <TelepathyQt4/ReferencedHandles>
#include <QGlib/Connect>
#include <QGst/ElementFactory>
#include <QGst/Pipeline>
#include <QGst/GhostPad>
#include <KDebug>

//BEGIN BaseSinkController

class BaseSinkControllerPrivate
{
public:
    BaseSinkControllerPrivate() : q_ptr(0) {}

    BaseSinkController *q_ptr;
    Tp::ContactPtr contact;
    QGst::BinPtr m_bin;
};

BaseSinkController::BaseSinkController(BaseSinkControllerPrivate *d, QObject *parent)
    : QObject(parent), d_ptr(d)
{
    d_ptr->q_ptr = this;
}

BaseSinkController::~BaseSinkController()
{
    delete d_ptr;
}

Tp::ContactPtr BaseSinkController::contact() const
{
    Q_D(const BaseSinkController);
    return d->contact;
}

//END BaseSinkController
//BEGIN AudioSinkController

class AudioSinkControllerPrivate : public BaseSinkControllerPrivate
{
public:
    QGst::PadPtr m_adderRequestPad;
    VolumeController *m_volumeController;
};

AudioSinkController::AudioSinkController(AudioSinkControllerPrivate *d, QObject *parent)
    : BaseSinkController(d, parent)
{
}

VolumeController *AudioSinkController::volumeController() const
{
    Q_D(const AudioSinkController);
    return d->m_volumeController;
}

//END AudioSinkController
//BEGIN VideoSinkController

class VideoSinkControllerPrivate : public BaseSinkControllerPrivate
{
public:
    VideoSinkControllerPrivate()
        : BaseSinkControllerPrivate(), m_padNameCounter(0) {}

    //<ghost src pad, tee request src pad>
    QHash<QGst::PadPtr, QGst::PadPtr> m_pads;
    QGst::ElementPtr m_tee;
    uint m_padNameCounter;
};

VideoSinkController::VideoSinkController(VideoSinkControllerPrivate *d, QObject *parent)
    : BaseSinkController(d, parent)
{
}

QGst::PadPtr VideoSinkController::requestSrcPad()
{
    Q_D(VideoSinkController);

    QString newPadName = QString("src%1").arg(d->m_padNameCounter);
    d->m_padNameCounter++;

    QGst::PadPtr teeSrcPad = d->m_tee->getRequestPad("src%d");
    QGst::PadPtr ghostSrcPad = QGst::GhostPad::create(teeSrcPad, newPadName.toAscii());

    ghostSrcPad->setActive(true);
    d->m_bin->addPad(ghostSrcPad);
    d->m_pads.insert(ghostSrcPad, teeSrcPad);

    return ghostSrcPad;
}

void VideoSinkController::releaseSrcPad(const QGst::PadPtr & pad)
{
    Q_D(VideoSinkController);

    QGst::PadPtr teeSrcPad = d->m_pads.take(pad);
    Q_ASSERT(!teeSrcPad.isNull());

    d->m_bin->removePad(pad);
    d->m_tee->releaseRequestPad(teeSrcPad);
}

//END VideoSinkController
//BEGIN BaseSinkManager

BaseSinkManager::BaseSinkManager(QObject *parent)
    : QObject(parent)
{
}

void BaseSinkManager::handleNewSinkPad(uint contactHandle, const QGst::PadPtr & pad)
{
    kDebug() << "New src pad" << pad->name() << "from handle" << contactHandle;

    //link the pad
    BaseSinkControllerPrivate *priv = createControllerPrivate(pad);

    //notify when the pad is unlinked (probably because it was removed from the fsconference)
    QGlib::connect(pad, "unlinked", this, &BaseSinkManager::onPadUnlinked, QGlib::PassSender);

    m_mutex.lock();
    m_controllersWaitingForContact.insert(contactHandle, priv);
    m_controllers.insert(pad, priv);
    m_mutex.unlock();

    //continue processing from the main thread
    QMetaObject::invokeMethod(this, "handleNewSinkPadAsync", Qt::QueuedConnection,
                              Q_ARG(uint, contactHandle));
}

void BaseSinkManager::handleNewSinkPadAsync(uint contactHandle)
{
    QMutexLocker l(&m_mutex);

    BaseSinkControllerPrivate *priv = m_controllersWaitingForContact.value(contactHandle);
    if (KDE_ISUNLIKELY(!priv)) {
        //just in case the pad was unlinked before we reached this slot
        kDebug() << "Not handling new pad. The pad was unlinked too early.";
        return;
    }

    if (m_content) {
        kDebug() << "Content exists. Looking for contact...";

        Q_FOREACH(const Tpy::CallStreamPtr & stream, m_content->streams()) {
            Q_FOREACH(const Tp::ContactPtr & contact, stream->members()) {
                if (contact->handle()[0] == contactHandle) {
                    kDebug() << "Found contact" << contact;

                    m_controllersWaitingForContact.remove(contactHandle);
                    priv->contact = contact;

                    BaseSinkController *c = createFullController(priv);
                    connect(c, SIGNAL(destroyed(QObject*)),
                            SLOT(onControllerAboutToBeDestroyed(QObject*)));
                    Q_EMIT controllerCreated(c);
                    return;
                }
            }
        }
    }

    kDebug() << "Contact not found. Waiting for tp-qt4 to sync with dbus";
}

void BaseSinkManager::setCallContent(const Tpy::CallContentPtr & callContent)
{
    m_content = callContent;
    connect(m_content.data(), SIGNAL(streamAdded(Tpy::CallStreamPtr)),
            SLOT(onStreamAdded(Tpy::CallStreamPtr)));
}

void BaseSinkManager::onStreamAdded(const Tpy::CallStreamPtr & stream)
{
    connect(stream.data(), SIGNAL(remoteSendingStateChanged(QHash<Tp::ContactPtr,Tpy::SendingState>)),
            SLOT(onRemoteSendingStateChanged(QHash<Tp::ContactPtr,Tpy::SendingState>)));
}

void BaseSinkManager::onRemoteSendingStateChanged(const QHash<Tp::ContactPtr, Tpy::SendingState> & states)
{
    QMutexLocker l(&m_mutex);

    if (!m_controllersWaitingForContact.isEmpty()) {
        Q_FOREACH(const Tp::ContactPtr & contact, states.keys()) {
            if (m_controllersWaitingForContact.contains(contact->handle()[0])) {
                kDebug() << "Found contact" << contact;

                BaseSinkControllerPrivate *priv = m_controllersWaitingForContact.take(contact->handle()[0]);
                priv->contact = contact;

                BaseSinkController *c = createFullController(priv);
                connect(c, SIGNAL(destroyed(QObject*)),
                        SLOT(onControllerAboutToBeDestroyed(QObject*)));
                Q_EMIT controllerCreated(c);
                return;
            }
        }
    }
}

void BaseSinkManager::onControllerAboutToBeDestroyed(QObject *controller)
{
    Q_EMIT controllerDestroyed(static_cast<BaseSinkController*>(controller));
}

void BaseSinkManager::unlinkAllPads()
{
    QMutexLocker l(&m_mutex);

    QHashIterator<QGst::PadPtr, BaseSinkControllerPrivate*> it(m_controllers);
    while (it.hasNext()) {
        it.next();

        QGst::PadPtr srcPad = it.key();
        QGst::PadPtr sinkPad = srcPad->peer();

        QGlib::disconnect(srcPad, 0, this, 0);
        srcPad->unlink(sinkPad);

        releaseControllerData(it.value());
        delete it.value()->q_ptr;
    }
    m_controllers.clear();
}

void BaseSinkManager::onPadUnlinked(const QGst::PadPtr & srcPad)
{
    QMutexLocker l(&m_mutex);

    kDebug() << srcPad->name() << "was unlinked.";
    kDebug() << "Current thread:" << QThread::currentThread()
             << "Main thread:" << QCoreApplication::instance()->thread();

    BaseSinkControllerPrivate *priv = m_controllers.value(srcPad);

    if (KDE_ISUNLIKELY(!priv)) {
        //just in case unlinkAllPads() locked the mutex first...
        kDebug() << "Looks like unlinkAllPads() wins...";
        return;
    }

    //check if the controller still has no public class constructed
    QMutableHashIterator<uint, BaseSinkControllerPrivate*> it(m_controllersWaitingForContact);
    while (it.hasNext()) {
        it.next();
        if (it.value() == priv) {
            Q_ASSERT(priv->q_ptr == NULL);

            releaseControllerData(priv);
            delete priv; //delete here. There is no public class to delete it.

            it.remove();
            m_controllers.remove(srcPad);

            return;
        }
    }

    //ok, the normal case now: the controller has a public class
    Q_ASSERT(priv->q_ptr != NULL);
    releaseControllerData(priv);
    QMetaObject::invokeMethod(priv->q_ptr, "deleteLater"); //delete from the main thread
}

//END BaseSinkManager
//BEGIN AudioSinkManager

AudioSinkManager::AudioSinkManager(const QGst::PipelinePtr & pipeline, QObject *parent)
    : BaseSinkManager(parent),
      m_pipeline(pipeline),
      m_sinkRefCount(0)
{
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
    m_sink->setState(QGst::StateReady);
    m_adder->setState(QGst::StateReady);
}

AudioSinkManager::~AudioSinkManager()
{
    Q_ASSERT(m_sinkRefCount == 0);
    m_adder->setState(QGst::StateNull);
    m_sink->setState(QGst::StateNull);
    m_adder->unlink(m_sink);
    m_pipeline->remove(m_adder);
    m_pipeline->remove(m_sink);
}

BaseSinkControllerPrivate *AudioSinkManager::createControllerPrivate(const QGst::PadPtr & srcPad)
{
    AudioSinkControllerPrivate *priv = new AudioSinkControllerPrivate;
    priv->m_bin = QGst::Bin::create();
    QGst::ElementPtr volume = QGst::ElementFactory::make("volume");
    QGst::ElementPtr audioConvert = QGst::ElementFactory::make("audioconvert");
    QGst::ElementPtr audioResample = QGst::ElementFactory::make("audioresample");
    QGst::ElementPtr level = QGst::ElementFactory::make("level");

    priv->m_bin->add(volume, audioConvert, audioResample, level);
    QGst::Element::linkMany(volume, audioConvert, audioResample, level);

    QGst::PadPtr binSinkPad = QGst::GhostPad::create(volume->getStaticPad("sink"), "sink");
    QGst::PadPtr binSrcPad = QGst::GhostPad::create(level->getStaticPad("src"), "src");
    priv->m_bin->addPad(binSinkPad);
    priv->m_bin->addPad(binSrcPad);

    refSink();
    m_pipeline->add(priv->m_bin);
    priv->m_bin->syncStateWithParent();

    priv->m_adderRequestPad = m_adder->getRequestPad("sink%d");
    binSinkPad->link(priv->m_adderRequestPad);
    srcPad->link(binSinkPad);

    return priv;
}

BaseSinkController *AudioSinkManager::createFullController(BaseSinkControllerPrivate *priv)
{
    AudioSinkControllerPrivate *apriv = static_cast<AudioSinkControllerPrivate*>(priv);
    AudioSinkController *c = new AudioSinkController(apriv, this);
    apriv->m_volumeController = new VolumeController(c);
    apriv->m_volumeController->setElement(apriv->m_bin->getElementByInterface<QGst::StreamVolume>());
    return c;
}

void AudioSinkManager::releaseControllerData(BaseSinkControllerPrivate *priv)
{
    AudioSinkControllerPrivate *apriv = static_cast<AudioSinkControllerPrivate*>(priv);
    apriv->m_bin->getStaticPad("src")->unlink(apriv->m_adderRequestPad);
    apriv->m_bin->setState(QGst::StateNull);
    apriv->m_bin.clear();

    m_adder->releaseRequestPad(apriv->m_adderRequestPad);
    apriv->m_adderRequestPad.clear();

    unrefSink();
}

void AudioSinkManager::refSink()
{
    QMutexLocker l(&m_mutex);
    if (m_sinkRefCount++ == 0) {
        m_sink->syncStateWithParent();
        m_adder->syncStateWithParent();
    }
}

void AudioSinkManager::unrefSink()
{
    QMutexLocker l(&m_mutex);
    if (--m_sinkRefCount == 0) {
        m_adder->setState(QGst::StateReady);
        m_sink->setState(QGst::StateReady);
    }
}

//END AudioSinkManager
//BEGIN VideoSinkManager

VideoSinkManager::VideoSinkManager(const QGst::PipelinePtr & pipeline, QObject *parent)
    : BaseSinkManager(parent),
      m_pipeline(pipeline)
{
}

BaseSinkControllerPrivate *VideoSinkManager::createControllerPrivate(const QGst::PadPtr & srcPad)
{
    VideoSinkControllerPrivate *priv = new VideoSinkControllerPrivate;
    priv->m_bin = QGst::Bin::create();
    priv->m_tee = QGst::ElementFactory::make("tee");

    QGst::ElementPtr fakesink = QGst::ElementFactory::make("fakesink");
    fakesink->setProperty("sync", false);
    fakesink->setProperty("async", false);
    fakesink->setProperty("silent", true);
    fakesink->setProperty("enable-last-buffer", false);

    priv->m_bin->add(priv->m_tee, fakesink);
    priv->m_tee->getRequestPad("src%d")->link(fakesink->getStaticPad("sink"));

    QGst::PadPtr binSinkPad = QGst::GhostPad::create(priv->m_tee->getStaticPad("sink"), "sink");
    priv->m_bin->addPad(binSinkPad);

    m_pipeline->add(priv->m_bin);
    priv->m_bin->syncStateWithParent();

    srcPad->link(binSinkPad);
    return priv;
}

BaseSinkController *VideoSinkManager::createFullController(BaseSinkControllerPrivate *priv)
{
    VideoSinkControllerPrivate *vpriv = static_cast<VideoSinkControllerPrivate*>(priv);
    VideoSinkController *c = new VideoSinkController(vpriv, this);
    return c;
}

void VideoSinkManager::releaseControllerData(BaseSinkControllerPrivate *priv)
{
    VideoSinkControllerPrivate *vpriv = static_cast<VideoSinkControllerPrivate*>(priv);
    vpriv->m_bin->setState(QGst::StateNull);
    m_pipeline->remove(vpriv->m_bin);
}

//END VideoSinkManager

#include "sinkcontrollers.moc"
#include "sinkcontrollers_p.moc"
