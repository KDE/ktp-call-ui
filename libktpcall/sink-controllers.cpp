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
#include "sink-controllers.h"
#include <QGst/ElementFactory>
#include <QGst/Pipeline>
#include <QGst/GhostPad>
#include <KDebug>

//BEGIN BaseSinkController

Tp::ContactPtr BaseSinkController::contact() const
{
    return m_contact;
}

void BaseSinkController::initFromMainThread(const Tp::ContactPtr & contact)
{
    m_contact = contact;
}

void BaseSinkController::releaseFromStreamingThread(const QGst::PipelinePtr & pipeline)
{
    m_bin->setState(QGst::StateNull);
    pipeline->remove(m_bin);
    m_bin.clear();
}

//END BaseSinkController
//BEGIN AudioSinkController

AudioSinkController::AudioSinkController(const QGst::PadPtr & adderSinkPad)
    : m_adderRequestPad(adderSinkPad),
      m_volumeController(NULL)
{
}

AudioSinkController::~AudioSinkController()
{
    delete m_volumeController;
}

QGst::PadPtr AudioSinkController::adderRequestPad() const
{
    return m_adderRequestPad;
}

VolumeController *AudioSinkController::volumeController() const
{
    return m_volumeController;
}

void AudioSinkController::initFromStreamingThread(const QGst::PadPtr & srcPad,
                                                  const QGst::PipelinePtr & pipeline)
{
    m_bin = QGst::Bin::fromDescription(
        "volume ! "
        "audioconvert ! "
        "audioresample ! "
        "level"
    );

    pipeline->add(m_bin);
    m_bin->syncStateWithParent();

    m_bin->getStaticPad("src")->link(m_adderRequestPad);
    srcPad->link(m_bin->getStaticPad("sink"));
}

void AudioSinkController::initFromMainThread(const Tp::ContactPtr & contact)
{
    m_volumeController = new VolumeController;
    m_volumeController->setElement(m_bin->getElementByInterface<QGst::StreamVolume>());

    BaseSinkController::initFromMainThread(contact);
}

void AudioSinkController::releaseFromStreamingThread(const QGst::PipelinePtr & pipeline)
{
    m_bin->getStaticPad("src")->unlink(m_adderRequestPad);
    m_adderRequestPad.clear();

    BaseSinkController::releaseFromStreamingThread(pipeline);
}

//END AudioSinkController
//BEGIN VideoSinkController

VideoSinkController::VideoSinkController()
    : m_padNameCounter(0)
{
}

QGst::PadPtr VideoSinkController::requestSrcPad()
{
    QString newPadName = QString("src%1").arg(m_padNameCounter);
    m_padNameCounter++;

    QGst::PadPtr teeSrcPad = m_tee->getRequestPad("src%d");
    QGst::PadPtr ghostSrcPad = QGst::GhostPad::create(teeSrcPad, newPadName.toAscii());

    ghostSrcPad->setActive(true);
    m_bin->addPad(ghostSrcPad);
    m_pads.insert(ghostSrcPad, teeSrcPad);

    return ghostSrcPad;
}

void VideoSinkController::releaseSrcPad(const QGst::PadPtr & pad)
{
    QGst::PadPtr teeSrcPad = m_pads.take(pad);
    Q_ASSERT(!teeSrcPad.isNull());

    m_bin->removePad(pad);
    m_tee->releaseRequestPad(teeSrcPad);
}

void VideoSinkController::initFromStreamingThread(const QGst::PadPtr & srcPad,
                                                  const QGst::PipelinePtr & pipeline)
{
    m_bin = QGst::Bin::create();
    m_tee = QGst::ElementFactory::make("tee");

    QGst::ElementPtr fakesink = QGst::ElementFactory::make("fakesink");
    fakesink->setProperty("sync", false);
    fakesink->setProperty("async", false);
    fakesink->setProperty("silent", true);
    fakesink->setProperty("enable-last-buffer", false);

    m_bin->add(m_tee, fakesink);
    m_tee->getRequestPad("src%d")->link(fakesink->getStaticPad("sink"));

    QGst::PadPtr binSinkPad = QGst::GhostPad::create(m_tee->getStaticPad("sink"), "sink");
    m_bin->addPad(binSinkPad);

    pipeline->add(m_bin);
    m_bin->syncStateWithParent();

    srcPad->link(binSinkPad);
}


//END VideoSinkController
