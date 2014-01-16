/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
    Copyright (C) 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tf-audio-content-handler.h"
#include "sink-controllers.h"
#include "device-element-factory.h"
#include "../volume-controller.h"

#include <QGlib/Error>
#include <QGst/ElementFactory>
#include <QGst/GhostPad>

#include <KDebug>

namespace KTpCallPrivate {

TfAudioContentHandler::TfAudioContentHandler(const QTf::ContentPtr & tfContent,
                                             TfChannelHandler *parent)
    : TfContentHandler(tfContent, parent),
      m_sinkRefCount(0)
{
    m_inputVolumeController = new VolumeController(this);
    m_outputVolumeController = new VolumeController(this);
}

TfAudioContentHandler::~TfAudioContentHandler()
{
}

VolumeController *TfAudioContentHandler::inputVolumeController() const
{
    return m_inputVolumeController;
}

VolumeController *TfAudioContentHandler::outputVolumeController() const
{
    return m_outputVolumeController;
}

BaseSinkController *TfAudioContentHandler::createSinkController(const QGst::PadPtr & srcPad)
{
    refSink();
    AudioSinkController *ctrl = new AudioSinkController(m_outputAdder->getRequestPad("sink_%u"));
    ctrl->initFromStreamingThread(srcPad, channelHandler()->pipeline());
    return ctrl;
}

void TfAudioContentHandler::releaseSinkControllerData(BaseSinkController *ctrl)
{
    AudioSinkController *actrl = static_cast<AudioSinkController*>(ctrl);
    QGst::PadPtr adderRequestPad = actrl->adderRequestPad();

    ctrl->releaseFromStreamingThread(channelHandler()->pipeline());

    m_outputAdder->releaseRequestPad(adderRequestPad);
    adderRequestPad.clear();

    unrefSink();
}

bool TfAudioContentHandler::startSending()
{
    QGst::ElementPtr src = DeviceElementFactory::makeAudioCaptureElement();
    if (!src) {
        kDebug() << "Could not initialize audio capture device";
        return false;
    }

    if (!createSrcBin(src)) {
        src->setState(QGst::StateNull); // DeviceElementFactory usually leaves src in StateReady
        return false;
    }

    // link to fsconference
    channelHandler()->pipeline()->add(m_srcBin);
    m_srcBin->getStaticPad("src")->link(tfContent()->property("sink-pad").get<QGst::PadPtr>());
    m_srcBin->syncStateWithParent();

    return true;
}

void TfAudioContentHandler::stopSending()
{
    m_inputVolumeController->setElement(QGst::StreamVolumePtr());

    if (m_srcBin) {
        m_srcBin->setStateLocked(true);
        m_srcBin->setState(QGst::StateNull);
        m_srcBin->getStaticPad("src")->unlink(tfContent()->property("sink-pad").get<QGst::PadPtr>());
        channelHandler()->pipeline()->remove(m_srcBin);
        m_srcBin.clear();
    }
}

void TfAudioContentHandler::onSinkCreated()
{
    if (m_outputVolume) {
        m_outputVolumeController->setElement(m_outputVolume.dynamicCast<QGst::StreamVolume>());
    } else {
        m_outputVolumeController->setElement(m_sink.dynamicCast<QGst::StreamVolume>());
    }
}

void TfAudioContentHandler::onSinkDestroyed()
{
    m_outputVolumeController->setElement(QGst::StreamVolumePtr());
}

void TfAudioContentHandler::refSink()
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
            m_sink->setProperty("enable-last-sample", false);
        }

        if (!m_sink.dynamicCast<QGst::StreamVolume>()) {
            m_outputVolume = QGst::ElementFactory::make("volume");
        }

        m_outputAdder = QGst::ElementFactory::make("liveadder");
        if (!m_outputAdder) {
            kWarning() << "Failed to create liveadder. Using adder. This may cause trouble...";
            m_outputAdder = QGst::ElementFactory::make("adder");
        }

        if (m_outputVolume) {
            channelHandler()->pipeline()->add(m_outputAdder, m_outputVolume, m_sink);
            QGst::Element::linkMany(m_outputAdder, m_outputVolume, m_sink);
        } else {
            channelHandler()->pipeline()->add(m_outputAdder, m_sink);
            m_outputAdder->link(m_sink);
        }

        m_sink->syncStateWithParent();
        if (m_outputVolume) {
            m_outputVolume->syncStateWithParent();
        }
        m_outputAdder->syncStateWithParent();

        QMetaObject::invokeMethod(this, "onSinkCreated");
    }
}

void TfAudioContentHandler::unrefSink()
{
    QMutexLocker l(&m_mutex);
    if (--m_sinkRefCount == 0) {
        m_outputAdder->setState(QGst::StateNull);
        m_sink->setState(QGst::StateNull);

        if (m_outputVolume) {
            m_outputVolume->setState(QGst::StateNull);
            QGst::Element::unlinkMany(m_outputAdder, m_outputVolume, m_sink);
            channelHandler()->pipeline()->remove(m_outputVolume);
            m_outputVolume.clear();
        } else {
            m_outputAdder->unlink(m_sink);
        }

        channelHandler()->pipeline()->remove(m_outputAdder);
        channelHandler()->pipeline()->remove(m_sink);
        m_outputAdder.clear();
        m_sink.clear();

        QMetaObject::invokeMethod(this, "onSinkDestroyed");
    }
}

bool TfAudioContentHandler::createSrcBin(const QGst::ElementPtr & src)
{
    //some unique id for this content - use the name that the CM gives to the content object
    QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);

    QString binDescription = QString(QLatin1String(
        "audioconvert name=input_bin_first_element_%1 ! "
        "audioresample ! "
        "volume name=input_volume_%1 ! "
        "level name=input_level_%1 ! "
        "audioconvert ! "
        "capsfilter caps=\"audio/x-raw-int,rate=[8000,16000];audio/x-raw-float,rate=[8000,16000]\" ! "
        "tee name=input_tee_%1 ! "
        "fakesink sync=false async=false silent=true enable-last-buffer=true")).arg(id);


    QGst::BinPtr bin;
    try {
        bin = QGst::Bin::fromDescription(binDescription, QGst::Bin::NoGhost);
    } catch (const QGlib::Error & err) {
        kWarning() << "Failed to create audio source bin" << err;
        return false;
    }

    // add the source
    QGst::ElementPtr firstElement = bin->getElementByName(
            QString(QLatin1String("input_bin_first_element_%1")).arg(id).toAscii());
    bin->add(src);
    if (!src->link(firstElement)) {
        kWarning() << "Failed to link audiosrc to audio src bin";
        return false;
    }

    // keep the volume element
    QGst::ElementPtr volume = bin->getElementByName(
            QString(QLatin1String("input_volume_%1")).arg(id).toAscii());
    m_inputVolumeController->setElement(volume.dynamicCast<QGst::StreamVolume>());

    // TODO level controller

    // add queue and src pad
    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");
    if (!queue) {
        kWarning() << "Failed to load the 'queue' gst element";
        return false;
    }
    bin->add(queue);

    QGst::ElementPtr tee = bin->getElementByName(
            QString(QLatin1String("input_tee_%1")).arg(id).toAscii());
    if (tee->getRequestPad("src_%u")->link(queue->getStaticPad("sink")) != QGst::PadLinkOk) {
        kWarning() << "Failed to link tee ! queue";
        return false;
    }

    bin->addPad(QGst::GhostPad::create(queue->getStaticPad("src"), "src"));

    m_srcBin = bin;
    return true;
}

} // KTpCallPrivate
