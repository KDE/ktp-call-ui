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

#include "tf-video-content-handler.h"
#include "sink-controllers.h"
#include "device-element-factory.h"
#include "video-sink-bin.h"

#include <QGlib/Connect>
#include <QGst/ElementFactory>
#include <QGst/GhostPad>
#include <QGst/FractionRange>
#include <QGst/Fraction>

#include <KDebug>

namespace KTpCallPrivate {

TfVideoContentHandler::TfVideoContentHandler(const QTf::ContentPtr & tfContent,
                                             TfChannelHandler *parent)
    : TfContentHandler(tfContent, parent),
      m_videoPreviewBin(NULL)
{
    QGlib::connect(tfContent, "restart-source", this, &TfVideoContentHandler::onRestartSource);
}

TfVideoContentHandler::~TfVideoContentHandler()
{
}

void TfVideoContentHandler::linkVideoPreviewSink(const QGst::ElementPtr & sink)
{
    kDebug();

    if (m_videoPreviewBin) {
        kWarning() << "video preview sink already linked - ignoring new preview sink";
        return;
    }

    QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);
    QString teeName = QString(QLatin1String("input_tee_%1")).arg(id);
    QGst::ElementPtr tee = m_srcBin->getElementByName(teeName.toAscii());

    QGst::PadPtr srcPad = tee->getRequestPad("src%d");
    m_videoPreviewBin = new VideoSinkBin(sink);

    m_srcBin->add(m_videoPreviewBin->bin());
    m_videoPreviewBin->bin()->syncStateWithParent();
    srcPad->link(m_videoPreviewBin->bin()->getStaticPad("sink"));
}

void TfVideoContentHandler::unlinkVideoPreviewSink()
{
    if (m_videoPreviewBin) {
        kDebug();

        QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);
        QString teeName = QString(QLatin1String("input_tee_%1")).arg(id);
        QGst::ElementPtr tee = m_srcBin->getElementByName(teeName.toAscii());

        QGst::PadPtr sinkPad = m_videoPreviewBin->bin()->getStaticPad("sink");
        QGst::PadPtr srcPad = sinkPad->peer();

        srcPad->unlink(sinkPad);
        m_videoPreviewBin->bin()->setStateLocked(true);
        m_videoPreviewBin->bin()->setState(QGst::StateNull);
        m_srcBin->remove(m_videoPreviewBin->bin());
        delete m_videoPreviewBin;
        m_videoPreviewBin = NULL;

        tee->releaseRequestPad(srcPad);
    }
}

BaseSinkController *TfVideoContentHandler::createSinkController(const QGst::PadPtr & srcPad)
{
    VideoSinkController *ctrl = new VideoSinkController;
    ctrl->initFromStreamingThread(srcPad, channelHandler()->pipeline());
    return ctrl;
}

void TfVideoContentHandler::releaseSinkControllerData(BaseSinkController *ctrl)
{
    ctrl->releaseFromStreamingThread(channelHandler()->pipeline());
}

bool TfVideoContentHandler::startSending()
{
    QGst::ElementPtr src = DeviceElementFactory::makeVideoCaptureElement();
    if (!src) {
        kDebug() << "Could not initialize video capture device";
        return false;
    }

    createSrcBin(src);

    // link to fsconference
    channelHandler()->pipeline()->add(m_srcBin);
    m_srcBin->getStaticPad("src")->link(tfContent()->property("sink-pad").get<QGst::PadPtr>());
    m_srcBin->syncStateWithParent();

    return true;
}

void TfVideoContentHandler::stopSending()
{
    unlinkVideoPreviewSink();

    if (m_srcBin) {
        m_srcBin->setStateLocked(true);
        m_srcBin->setState(QGst::StateNull);
        m_srcBin->getStaticPad("src")->unlink(tfContent()->property("sink-pad").get<QGst::PadPtr>());
        channelHandler()->pipeline()->remove(m_srcBin);
        m_srcBin.clear();
    }
}

void TfVideoContentHandler::createSrcBin(const QGst::ElementPtr & src)
{
    m_srcBin = QGst::Bin::create();

    //some unique id for this content - use the name that the CM gives to the content object
    QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);

    //videomaxrate drops frames to support the 15fps restriction
    //in the capsfilter if the camera cannot produce 15fps
    QGst::ElementPtr videomaxrate = QGst::ElementFactory::make("videomaxrate");

    //ffmpegcolorspace converts to yuv for postproc_tmpnoise
    //to work if the camera cannot produce yuv
    QGst::ElementPtr colorspace = QGst::ElementFactory::make("ffmpegcolorspace");

    //videoscale supports the 320x240 restriction in the capsfilter
    //if the camera cannot produce 320x240
    QGst::ElementPtr videoscale = QGst::ElementFactory::make("videoscale");

    //capsfilter restricts the output to 320x240 @ 15fps or whatever Content.I.VideoControl says
    QString capsfilterName = QString(QLatin1String("input_capsfilter_%1")).arg(id);
    QGst::ElementPtr capsfilter = QGst::ElementFactory::make("capsfilter", capsfilterName.toAscii());
    capsfilter->setProperty("caps", contentCaps());

    //postproc_tmpnoise reduces video noise to improve quality
    QGst::ElementPtr postproc = QGst::ElementFactory::make("postproc_tmpnoise");

    //tee to support fakesink + fsconference + video preview sink
    QString teeName = QString(QLatin1String("input_tee_%1")).arg(id);
    QGst::ElementPtr tee = QGst::ElementFactory::make("tee", teeName.toAscii());

    //fakesink silently "eats" frames to prevent the source from stopping in case there is no other sink
    QGst::ElementPtr fakesink = QGst::ElementFactory::make("fakesink");
    fakesink->setProperty("sync", false);
    fakesink->setProperty("async", false);
    fakesink->setProperty("silent", true);
    fakesink->setProperty("enable-last-buffer", false);

    //queue to support fsconference after the tee
    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");

    m_srcBin->add(src, colorspace, videoscale, capsfilter, tee, fakesink);

    // src ! (videomaxrate) ! colorspace
    if (videomaxrate) {
        m_srcBin->add(videomaxrate);
        QGst::Element::linkMany(src, videomaxrate, colorspace);
    } else {
        kDebug() << "NOT using videomaxrate";
        src->link(colorspace);
    }

    // colorspace ! videoscale ! capsfilter
    QGst::Element::linkMany(colorspace, videoscale, capsfilter);

    // capsfilter ! (postproc_tmpnoise) ! tee
    if (postproc) {
        m_srcBin->add(postproc);
        QGst::Element::linkMany(capsfilter, postproc, tee);
    } else {
        kDebug() << "NOT using postproc_tmpnoise";
        capsfilter->link(tee);
    }

    // tee ! fakesink
    tee->getRequestPad("src%d")->link(fakesink->getStaticPad("sink"));

    // tee ! queue
    tee->getRequestPad("src%d")->link(queue->getStaticPad("sink"));

    // create bin's src pad
    m_srcBin->addPad(QGst::GhostPad::create(queue->getStaticPad("src"), "src"));
}

QGst::CapsPtr TfVideoContentHandler::contentCaps() const
{
    // TfContent advertises the Content.I.VideoControl properties, if the interface exists,
    // otherwise it returns 0 for all of them
    uint width = tfContent()->property("width").toUInt();
    uint height = tfContent()->property("height").toUInt();
    if (width == 0 || height == 0) {
        width = 320;
        height = 240;
    }

    uint framerate = tfContent()->property("framerate").toUInt();
    if (framerate == 0) {
        framerate = 15;
    }

    QGst::Structure capsStruct("video/x-raw-yuv");
    capsStruct.setValue("width", width);
    capsStruct.setValue("height", height);
    capsStruct.setValue("framerate", QGst::Fraction(framerate, 1));

    QGst::CapsPtr caps = QGst::Caps::createEmpty();
    caps->appendStructure(capsStruct);
    return caps;
}

void TfVideoContentHandler::onRestartSource()
{
    if (m_srcBin) {
        QGst::CapsPtr caps = contentCaps();
        kDebug() << "restarting source with new caps" << caps;

        QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);
        QString capsfilterName = QString(QLatin1String("input_capsfilter_%1")).arg(id);
        QGst::ElementPtr capsfilter = m_srcBin->getElementByName(capsfilterName.toAscii());

        //stop src bin
        m_srcBin->setStateLocked(true);
        m_srcBin->setState(QGst::StateNull);

        //change caps
        capsfilter->setProperty("caps", caps);

        //reset the clock
        m_srcBin->setClock(channelHandler()->pipeline()->clock());

        //restart
        m_srcBin->setStateLocked(false);
        m_srcBin->syncStateWithParent();
    }
}

} // KTpCallPrivate
