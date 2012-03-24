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
#include "source-controllers.h"
#include "device-element-factory.h"
#include <QGst/ElementFactory>
#include <QGst/GhostPad>
#include <QGst/Structure>
#include <QGst/Caps>
#include <QGst/Fraction>
#include <KDebug>

//BEGIN BaseSourceController

BaseSourceController::BaseSourceController(const QGst::PipelinePtr & pipeline)
    : m_pipeline(pipeline),
      m_inputCtrlBin(NULL)
{
}

BaseSourceController::~BaseSourceController()
{
    if (m_inputCtrlBin) {
        destroyBin();
    }
}

QGst::PadPtr BaseSourceController::requestSrcPad()
{
    if (!m_inputCtrlBin) {
        createBin();
    }

    return m_inputCtrlBin->requestSrcPad();
}

void BaseSourceController::releaseSrcPad(const QGst::PadPtr & pad)
{
    m_inputCtrlBin->releaseSrcPad(pad);
}

void BaseSourceController::connectToSource()
{
    if (m_source.isNull()) {
        //create the source
        //TODO this should happen async, as some cameras take a long time to initialize
        m_source = makeRealSource();
        if (!m_source) {
            return;
        }
        m_inputCtrlBin->connectSource(m_source);
    }
    //else we are already connected
}

void BaseSourceController::disconnectFromSource()
{
    if (!m_source.isNull()) {
        releaseRealSource(); //release any pointers that the subclass keeps

        m_inputCtrlBin->disconnectSource();
        m_source.clear();
    }
    //else we are already disconnected
}

void BaseSourceController::createBin()
{
    m_inputCtrlBin = new InputControlBin(makeSilenceSource(), makeFilterBin());

    // play
    m_pipeline->add(m_inputCtrlBin->bin());
    m_inputCtrlBin->bin()->syncStateWithParent();
}

void BaseSourceController::destroyBin()
{
    disconnectFromSource();

    m_inputCtrlBin->bin()->setState(QGst::StateNull);
    m_pipeline->remove(m_inputCtrlBin->bin());

    delete m_inputCtrlBin;
}

//END BaseSourceController
//BEGIN AudioSourceController

AudioSourceController::AudioSourceController(const QGst::PipelinePtr & pipeline)
    : BaseSourceController(pipeline)
{
    m_volumeController = new VolumeController;
}

AudioSourceController::~AudioSourceController()
{
    delete m_volumeController;
}

VolumeController *AudioSourceController::volumeController() const
{
    return m_volumeController;
}

QGst::ElementPtr AudioSourceController::makeSilenceSource()
{
    QGst::ElementPtr src = QGst::ElementFactory::make("audiotestsrc");
    src->setProperty("wave", 4); //generate silence
    src->setProperty("is-live", true);
    return src;
}

QGst::ElementPtr AudioSourceController::makeRealSource()
{
    QGst::ElementPtr src = DeviceElementFactory::makeAudioCaptureElement();
    if (!src) {
        kDebug() << "Could not initialize audio capture device";
        return QGst::ElementPtr();
    }

    QGst::BinPtr bin = QGst::Bin::create();
    QGst::ElementPtr volume = QGst::ElementFactory::make("volume");
    QGst::ElementPtr level = QGst::ElementFactory::make("level");

    m_volumeController->setElement(volume.dynamicCast<QGst::StreamVolume>());

    bin->add(src, volume, level);
    QGst::Element::linkMany(src, volume, level);

    bin->addPad(QGst::GhostPad::create(level->getStaticPad("src"), "src"));

    return bin;
}

QGst::ElementPtr AudioSourceController::makeFilterBin()
{
    return QGst::Bin::fromDescription(
        "audiorate ! "
        "capsfilter caps=\"audio/x-raw-int,rate=[8000,16000];audio/x-raw-float,rate=[8000,16000]\""
    );
}

void AudioSourceController::releaseRealSource()
{
    m_volumeController->setElement(QGst::StreamVolumePtr());
}

//END AudioSourceController
//BEGIN VideoSourceController

VideoSourceController::VideoSourceController(const QGst::PipelinePtr & pipeline)
    : BaseSourceController(pipeline),
      m_videoSinkBin(NULL)
{
}

VideoSourceController::~VideoSourceController()
{
    unlinkVideoPreviewSink();
}

void VideoSourceController::linkVideoPreviewSink(const QGst::ElementPtr & sink)
{
    QGst::PadPtr srcPad = requestSrcPad();
    m_videoSinkBin = new VideoSinkBin(sink);

    m_pipeline->add(m_videoSinkBin->bin());
    m_videoSinkBin->bin()->syncStateWithParent();
    srcPad->link(m_videoSinkBin->bin()->getStaticPad("sink"));
}

void VideoSourceController::unlinkVideoPreviewSink()
{
    if (m_videoSinkBin) {
        QGst::PadPtr sinkPad = m_videoSinkBin->bin()->getStaticPad("sink");
        QGst::PadPtr srcPad = sinkPad->peer();

        srcPad->unlink(sinkPad);
        m_videoSinkBin->bin()->setState(QGst::StateNull);
        m_pipeline->remove(m_videoSinkBin->bin());
        delete m_videoSinkBin;
        m_videoSinkBin = NULL;

        releaseSrcPad(srcPad);
    }
}

QGst::ElementPtr VideoSourceController::makeSilenceSource()
{
    QGst::ElementPtr src = QGst::ElementFactory::make("videotestsrc");
    src->setProperty("pattern", 2); //black picture
    src->setProperty("is-live", true);
    return src;
}

QGst::ElementPtr VideoSourceController::makeRealSource()
{
    QGst::ElementPtr src = DeviceElementFactory::makeVideoCaptureElement();
    if (!src) {
        kDebug() << "Could not initialize video capture device";
        return QGst::ElementPtr();
    } else {
        return src;
    }

//TODO colorbalance support
//     QGst::ElementPtr realSrc;
//     QGst::BinPtr srcBin = src.dynamicCast<QGst::Bin>();
//     if (srcBin) {
//         realSrc = srcBin->childByIndex(0).dynamicCast<QGst::Element>();
//     } else {
//         realSrc = src;
//     }
//
//     QGst::ElementPtr videobalance;
//     m_colorBalance = realSrc.dynamicCast<QGst::ColorBalance>();
//     if (!m_colorBalance) {
//         kDebug() << "Source does not support color balance";
//         videobalance = QGst::ElementFactory::make("videobalance");
//         m_colorBalance = videobalance.dynamicCast<QGst::ColorBalance>();
//     }
}

QGst::ElementPtr VideoSourceController::makeFilterBin()
{
    QGst::BinPtr bin = QGst::Bin::create();

    //videomaxrate drops frames to support the 15fps restriction
    //in the capsfilter if the camera cannot produce 15fps
    QGst::ElementPtr videomaxrate = QGst::ElementFactory::make("videomaxrate");

    //ffmpegcolorspace converts to yuv for postproc_tmpnoise
    //to work if the camera cannot produce yuv
    QGst::ElementPtr colorspace = QGst::ElementFactory::make("ffmpegcolorspace");

    //videoscale supports the 320x240 restriction in the capsfilter
    //if the camera cannot produce 320x240
    QGst::ElementPtr videoscale = QGst::ElementFactory::make("videoscale");

    //capsfilter restricts the output to 320x240 @ 15fps
    QGst::ElementPtr capsfilter = QGst::ElementFactory::make("capsfilter");

    //postproc_tmpnoise reduces video noise to improve quality
    QGst::ElementPtr postproc = QGst::ElementFactory::make("postproc_tmpnoise");

    QGst::Structure capsStruct("video/x-raw-yuv");
    capsStruct.setValue("width", 320);
    capsStruct.setValue("height", 240);
    capsStruct.setValue("framerate", QGst::FractionRange(QGst::Fraction(0,1), QGst::Fraction(15,1)));

    QGst::CapsPtr caps = QGst::Caps::createEmpty();
    caps->appendStructure(capsStruct);
    capsfilter->setProperty("caps", caps);

    bin->add(colorspace, videoscale, capsfilter);

    /* videomaxrate and postproc_tmpnoise are optional */
    if (videomaxrate) {
        bin->add(videomaxrate);
        videomaxrate->link(colorspace);
        bin->addPad(QGst::GhostPad::create(videomaxrate->getStaticPad("sink"), "sink"));
    } else {
        kDebug() << "NOT using videomaxrate";
        bin->addPad(QGst::GhostPad::create(colorspace->getStaticPad("sink"), "sink"));
    }

    QGst::Element::linkMany(colorspace, videoscale, capsfilter);

    if (postproc) {
        bin->add(postproc);
        capsfilter->link(postproc);
        bin->addPad(QGst::GhostPad::create(postproc->getStaticPad("src"), "src"));
    } else {
        kDebug() << "NOT using postproc_tmpnoise";
        bin->addPad(QGst::GhostPad::create(capsfilter->getStaticPad("src"), "src"));
    }

    return bin;
}

//END VideoSourceController
