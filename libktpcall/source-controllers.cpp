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
#include "source-controllers_p.h"
#include "device-element-factory_p.h"
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
    m_inputCtrlBin = NULL;
    m_pipeline = pipeline;
}

BaseSourceControllerPrivate::~BaseSourceControllerPrivate()
{
    if (m_inputCtrlBin) {
        destroyBin();
    }
}

QGst::PadPtr BaseSourceControllerPrivate::requestSrcPad()
{
    if (!m_inputCtrlBin) {
        createBin();
    }

    return m_inputCtrlBin->requestSrcPad();
}

void BaseSourceControllerPrivate::releaseSrcPad(const QGst::PadPtr & pad)
{
    m_inputCtrlBin->releaseSrcPad(pad);
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
        m_inputCtrlBin->connectSource(m_source);

        Q_EMIT q->sourceEnabledChanged(true);
    }
    //else we are already connected
}

void BaseSourceControllerPrivate::disconnectFromSource()
{
    Q_Q(BaseSourceController);

    if (!m_source.isNull()) {
        releaseRealSource(); //release any pointers that the subclass keeps

        m_inputCtrlBin->disconnectSource();
        m_source.clear();

        Q_EMIT q->sourceEnabledChanged(false);
    }
    //else we are already disconnected
}

void BaseSourceControllerPrivate::createBin()
{
    m_inputCtrlBin = new InputControlBin(makeSilenceSource(), makeFilterBin());

    // play
    m_pipeline->add(m_inputCtrlBin->bin());
    m_inputCtrlBin->bin()->syncStateWithParent();
}

void BaseSourceControllerPrivate::destroyBin()
{
    disconnectFromSource();

    m_inputCtrlBin->bin()->setState(QGst::StateNull);
    m_pipeline->remove(m_inputCtrlBin->bin());

    delete m_inputCtrlBin;
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
    QGst::ElementPtr level = QGst::ElementFactory::make("level");

    m_volumeController->setElement(volume.dynamicCast<QGst::StreamVolume>());

    bin->add(src, volume, level);
    QGst::Element::linkMany(src, volume, level);

    bin->addPad(QGst::GhostPad::create(level->getStaticPad("src"), "src"));

    return bin;
}

QGst::ElementPtr AudioSourceControllerPrivate::makeFilterBin()
{
    return QGst::Bin::fromDescription(
        "audiorate ! "
        "capsfilter caps=\"audio/x-raw-int,rate=[8000,16000];audio/x-raw-float,rate=[8000,16000]\""
    );
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

QGst::ElementPtr VideoSourceControllerPrivate::makeFilterBin()
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

//END VideoSourceControllerPrivate

#include "moc_source-controllers.cpp"
