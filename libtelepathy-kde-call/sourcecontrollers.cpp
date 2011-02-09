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
#include "sourcecontrollers_p.h"
#include "deviceelementfactory_p.h"
#include <QGst/ElementFactory>
#include <QGst/GhostPad>
#include <QGst/Structure>
#include <QGst/Caps>
#include <QGst/Fraction>
#include <KDebug>

//BEGIN BaseSourceController

BaseSourceController::BaseSourceController(BaseSourceControllerPrivate *d, QObject *parent)
    : QObject(parent), d_ptr(d)
{
    d->q_ptr = this;
}

BaseSourceController::~BaseSourceController()
{
    delete d_ptr;
}

QGst::PadPtr BaseSourceController::requestSrcPad()
{
    Q_D(BaseSourceController);
    return d->requestSrcPad();
}

void BaseSourceController::releaseSrcPad(const QGst::PadPtr & pad)
{
    Q_D(BaseSourceController);
    return d->releaseSrcPad(pad);
}

bool BaseSourceController::sourceEnabled() const
{
    Q_D(const BaseSourceController);
    return !d->m_source.isNull();
}

void BaseSourceController::setSourceEnabled(bool enabled)
{
    Q_D(BaseSourceController);
    if (enabled) {
        d->connectToSource();
    } else {
        d->disconnectFromSource();
    }
}

//END BaseSourceController
//BEGIN BaseSourceControllerPrivate

BaseSourceControllerPrivate::BaseSourceControllerPrivate(const QGst::PipelinePtr & pipeline)
{
    m_pipeline = pipeline;
}

BaseSourceControllerPrivate::~BaseSourceControllerPrivate()
{
    m_pads.clear();
    if (m_bin) {
        destroyBin();
    }
}

QGst::PadPtr BaseSourceControllerPrivate::requestSrcPad()
{
    static uint padNameCounter = 0;

    if (!m_bin) {
        createBin();
    }

    QString newPadName = QString("src%1").arg(padNameCounter);
    padNameCounter++;

    QGst::PadPtr teeSrcPad = m_tee->getRequestPad("src%d");
    QGst::PadPtr ghostSrcPad = QGst::GhostPad::create(teeSrcPad, newPadName.toAscii());

    ghostSrcPad->setActive(true);
    m_bin->addPad(ghostSrcPad);
    m_pads.insert(ghostSrcPad, teeSrcPad);

    return ghostSrcPad;
}

void BaseSourceControllerPrivate::releaseSrcPad(const QGst::PadPtr & pad)
{
    QGst::PadPtr teeSrcPad = m_pads.take(pad);
    Q_ASSERT(!teeSrcPad.isNull());

    m_bin->removePad(pad);
    m_tee->releaseRequestPad(teeSrcPad);

    if (m_pads.isEmpty()) {
        destroyBin();
    }
}

void BaseSourceControllerPrivate::connectToSource()
{
    Q_Q(BaseSourceController);

    if (m_source.isNull()) {
        //create the source
        //TODO this should happen async, as some cameras take a long time to initialize
        m_source = makeRealSource();
        if (!m_source) {
            return;
        }
        m_bin->add(m_source);

        //connect the source to the input-selector sink pad
        m_selectorSinkPad = m_inputSelector->getRequestPad("sink%d");
        m_source->getStaticPad("src")->link(m_selectorSinkPad);

        //set the source to playing state
        m_source->syncStateWithParent();

        //swap sources
        switchToRealSource();

        Q_EMIT q->sourceEnabledChanged(true);
    }
    //else we are already connected
}

void BaseSourceControllerPrivate::disconnectFromSource()
{
    Q_Q(BaseSourceController);

    if (!m_source.isNull()) {
        //swap sources
        switchToSilenceSource();

        //disconnect the source from the input-selector sink pad
        m_source->getStaticPad("src")->unlink(m_selectorSinkPad);
        m_inputSelector->releaseRequestPad(m_selectorSinkPad);
        m_selectorSinkPad.clear();

        //destroy the source
        releaseRealSource(); //release any pointers that the subclass keeps
        m_source->setState(QGst::StateNull);
        m_bin->remove(m_source);
        m_source.clear();

        Q_EMIT q->sourceEnabledChanged(false);
    }
    //else we are already disconnected
}

void BaseSourceControllerPrivate::createBin()
{
    m_bin = QGst::Bin::create();
    m_silenceSource = makeSilenceSource();
    m_inputSelector = QGst::ElementFactory::make("input-selector");
    m_tee = QGst::ElementFactory::make("tee");
    m_fakesink = QGst::ElementFactory::make("fakesink");
    m_fakesink->setProperty("sync", false);
    m_fakesink->setProperty("async", false);
    m_fakesink->setProperty("silent", true);
    m_fakesink->setProperty("enable-last-buffer", false);

    m_bin->add(m_silenceSource, m_inputSelector, m_tee, m_fakesink);

    // silencesource ! input-selector
    QGst::PadPtr selectorSinkPad = m_inputSelector->getRequestPad("sink%d");
    m_silenceSource->getStaticPad("src")->link(selectorSinkPad);

    // input-selector ! tee
    m_inputSelector->link(m_tee);

    // tee ! fakesink
    m_tee->getRequestPad("src%d")->link(m_fakesink->getStaticPad("sink"));

    // play
    m_pipeline->add(m_bin);
    m_bin->syncStateWithParent();
}

void BaseSourceControllerPrivate::destroyBin()
{
    disconnectFromSource();

    m_bin->setState(QGst::StateNull);
    m_pipeline->remove(m_bin);

    m_fakesink.clear();
    m_tee.clear();
    m_inputSelector.clear();
    m_silenceSource.clear();
    m_bin.clear();
}

void BaseSourceControllerPrivate::switchToRealSource()
{
    //wait for data to arrive on the source src pad
    //required for synchronization with the streaming thread
    QGst::PadPtr sourceSrcPad = m_source->getStaticPad("src");
    sourceSrcPad->setBlocked(true);
    sourceSrcPad->setBlocked(false);

    //switch inputs
    m_inputSelector->setProperty("active-pad", m_selectorSinkPad);
}

void BaseSourceControllerPrivate::switchToSilenceSource()
{
    //switch inputs
    m_inputSelector->setProperty("active-pad", m_inputSelector->getStaticPad("sink0"));

    //wait for data to arrive on the inputselector src pad
    //required for synchronization with the streaming thread
    QGst::PadPtr selectorSrcPad = m_inputSelector->getStaticPad("src");
    selectorSrcPad->setBlocked(true);
    selectorSrcPad->setBlocked(false);
}

//END BaseSourceControllerPrivate
//BEGIN AudioSourceController

AudioSourceController::AudioSourceController(const QGst::PipelinePtr & pipeline, QObject *parent)
    : BaseSourceController(new AudioSourceControllerPrivate(pipeline), parent)
{
}

VolumeController *AudioSourceController::volumeController() const
{
    Q_D(const AudioSourceController);
    return d->m_volumeController;
}

//END AudioSourceController
//BEGIN AudioSourceControllerPrivate

AudioSourceControllerPrivate::AudioSourceControllerPrivate(const QGst::PipelinePtr & pipeline)
    : BaseSourceControllerPrivate(pipeline)
{
    m_volumeController = new VolumeController;
}

QGst::ElementPtr AudioSourceControllerPrivate::makeSilenceSource()
{
    QGst::ElementPtr src = QGst::ElementFactory::make("audiotestsrc");
    src->setProperty("wave", 4); //generate silence
    src->setProperty("is-live", true);
    return src;
}

QGst::ElementPtr AudioSourceControllerPrivate::makeRealSource()
{
    QGst::ElementPtr src = DeviceElementFactory::makeAudioCaptureElement();
    if (!src) {
        kDebug() << "Could not initialize audio capture device";
        return QGst::ElementPtr();
    }

    QGst::BinPtr bin = QGst::Bin::create();
    QGst::ElementPtr volume = QGst::ElementFactory::make("volume");
    QGst::ElementPtr audioConvert = QGst::ElementFactory::make("audioconvert");
    QGst::ElementPtr audioResample = QGst::ElementFactory::make("audioresample");
    QGst::ElementPtr level = QGst::ElementFactory::make("level");

    m_volumeController->setElement(volume.dynamicCast<QGst::StreamVolume>());

    bin->add(src, volume, audioConvert, audioResample, level);
    QGst::Element::linkMany(src, volume, audioConvert, audioResample, level);

    bin->addPad(QGst::GhostPad::create(level->getStaticPad("src"), "src"));

    return bin;
}

void AudioSourceControllerPrivate::releaseRealSource()
{
    m_volumeController->setElement(QGst::StreamVolumePtr());
}

//END AudioSourceControllerPrivate
//BEGIN VideoSourceController

VideoSourceController::VideoSourceController(const QGst::PipelinePtr & pipeline, QObject *parent)
    : BaseSourceController(new VideoSourceControllerPrivate(pipeline), parent)
{
}

//END VideoSourceController
//BEGIN VideoSourceControllerPrivate

VideoSourceControllerPrivate::VideoSourceControllerPrivate(const QGst::PipelinePtr & pipeline)
    : BaseSourceControllerPrivate(pipeline)
{
}

QGst::ElementPtr VideoSourceControllerPrivate::makeSilenceSource()
{
    QGst::ElementPtr src = QGst::ElementFactory::make("videotestsrc");
    src->setProperty("pattern", 2); //black picture
    src->setProperty("is-live", true);
    return src;
}

QGst::ElementPtr VideoSourceControllerPrivate::makeRealSource()
{
    /*
     * (source) ! videomaxrate (optional) ! videobalance (optional)
     * ! ffmpegcolorspace ! videoscale ! capsfilter ! postproc_tmpnoise (optional)
     */

    QGst::ElementPtr src = DeviceElementFactory::makeVideoCaptureElement();
    if (!src) {
        kDebug() << "Could not initialize video capture device";
        return QGst::ElementPtr();
    }

    QGst::BinPtr bin = QGst::Bin::create();
    QGst::ElementPtr videomaxrate = QGst::ElementFactory::make("videomaxrate");
    QGst::ElementPtr colorspace = QGst::ElementFactory::make("ffmpegcolorspace");
    QGst::ElementPtr videoscale = QGst::ElementFactory::make("videoscale");
    QGst::ElementPtr capsfilter = QGst::ElementFactory::make("capsfilter");
    QGst::ElementPtr postproc = QGst::ElementFactory::make("postproc_tmpnoise");

    QGst::ElementPtr realSrc;
    QGst::BinPtr srcBin = src.dynamicCast<QGst::Bin>();
    if (srcBin) {
        realSrc = srcBin->childByIndex(0).dynamicCast<QGst::Element>();
    } else {
        realSrc = src;
    }

    QGst::ElementPtr videobalance;
//     m_colorBalance = realSrc.dynamicCast<QGst::ColorBalance>();
//     if (!m_colorBalance) {
//         kDebug() << "Source does not support color balance";
//         videobalance = QGst::ElementFactory::make("videobalance");
//         m_colorBalance = videobalance.dynamicCast<QGst::ColorBalance>();
//     }

    //TODO have methods to select preferred resolution + query source
    QGst::Structure capsStruct("video/x-raw-yuv");
    capsStruct.setValue("width", 320);
    capsStruct.setValue("height", 240);

    /* videomaxrate is optional, since it is not always available */
    if (videomaxrate) {
        bin->add(videomaxrate);
    } else {
        kDebug() << "NOT using videomaxrate";
        //TODO query source to get supported framerates
        capsStruct.setValue("framerate", QGst::Fraction(15,1));
    }

    if (videobalance) {
        kDebug() << "Using videobalance";
        bin->add(videobalance);
    }

    QGst::CapsPtr caps = QGst::Caps::createEmpty();
    caps->appendStructure(capsStruct);
    capsfilter->setProperty("caps", caps);

    bin->add(colorspace, videoscale, capsfilter);
    if (postproc) {
        bin->add(postproc);
    }

    if (videomaxrate) {
        if (videobalance) {
            videomaxrate->link(videobalance);
        } else {
            videomaxrate->link(colorspace);
        }
    } else {
        if (videobalance) {
            videobalance->link(colorspace);
        }
    }
    QGst::Element::linkMany(colorspace, videoscale, capsfilter);
    if (postproc) {
        capsfilter->link(postproc);
        bin->addPad(QGst::GhostPad::create(postproc->getStaticPad("src"), "src"));
    } else {
        kDebug() << "NOT using postproc_tmpnoise";
        bin->addPad(QGst::GhostPad::create(capsfilter->getStaticPad("src"), "src"));
    }

    return bin;
}

//END VideoSourceControllerPrivate

#include "sourcecontrollers.moc"
