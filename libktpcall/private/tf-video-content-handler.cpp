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
#include "libktpcall_debug.h"

#include <QGlib/Connect>
#include <QGst/Clock>
#include <QGst/ElementFactory>
#include <QGst/GhostPad>
#include <QGst/FractionRange>
#include <QGst/Fraction>

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
    qCDebug(LIBKTPCALL);

    if (m_videoPreviewBin) {
        qCWarning(LIBKTPCALL) << "video preview sink already linked - ignoring new preview sink";
        return;
    }

    QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);
    QString teeName = QString(QLatin1String("input_tee_%1")).arg(id);
    QGst::ElementPtr tee = m_srcBin->getElementByName(teeName.toLatin1());

    QGst::PadPtr srcPad = tee->getRequestPad("src_%u");
    m_videoPreviewBin = new VideoSinkBin(sink);

    m_srcBin->add(m_videoPreviewBin->bin());
    m_videoPreviewBin->bin()->syncStateWithParent();
    srcPad->link(m_videoPreviewBin->bin()->getStaticPad("sink"));
}

void TfVideoContentHandler::unlinkVideoPreviewSink()
{
    if (m_videoPreviewBin) {
        qCDebug(LIBKTPCALL);

        QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);
        QString teeName = QString(QLatin1String("input_tee_%1")).arg(id);
        QGst::ElementPtr tee = m_srcBin->getElementByName(teeName.toLatin1());

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
        qCCritical(LIBKTPCALL) << "Could not initialize video capture device";
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

void TfVideoContentHandler::stopSending()
{
    unlinkVideoPreviewSink();

    if (m_srcBin) {
        m_srcBin->setStateLocked(true);
        m_srcBin->setState(QGst::StateNull);
        m_srcBin->getStaticPad("src")->unlink(tfContent()->property("sink-pad").get<QGst::PadPtr>());
        if (channelHandler()->pipeline()) {
            channelHandler()->pipeline()->remove(m_srcBin);
        }
        m_srcBin.clear();
    }
}

bool TfVideoContentHandler::createSrcBin(const QGst::ElementPtr & src)
{
    //some unique id for this content - use the name that the CM gives to the content object
    QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);

    //videorate drops frames to support the 15fps restriction
    //in the capsfilter if the camera cannot produce 15fps
    QGst::ElementPtr videorate = QGst::ElementFactory::make("videorate");
    videorate->setProperty("max-rate", 15);

    //videoscale supports the 320x240 restriction in the capsfilter
    //if the camera cannot produce 320x240
    QGst::ElementPtr videoscale = QGst::ElementFactory::make("videoscale");

    //ffmpegcolorspace converts to yuv for postproc_tmpnoise
    //to work if the camera cannot produce yuv
    QGst::ElementPtr colorspace = QGst::ElementFactory::make("videoconvert");

    //capsfilter restricts the output to 320x240 @ 15fps or whatever Content.I.VideoControl says
    QString capsfilterName = QString(QLatin1String("input_capsfilter_%1")).arg(id);
    QGst::ElementPtr capsfilter = QGst::ElementFactory::make("capsfilter", capsfilterName.toLatin1());
    capsfilter->setProperty("caps", contentCaps());

    qCDebug(LIBKTPCALL) << "Using video src caps" << capsfilter->property("caps").get<QGst::CapsPtr>();

    //tee to support fakesink + fsconference + video preview sink
    QString teeName = QString(QLatin1String("input_tee_%1")).arg(id);
    QGst::ElementPtr tee = QGst::ElementFactory::make("tee", teeName.toLatin1());

    //fakesink silently "eats" frames to prevent the source from stopping in case there is no other sink
    QGst::ElementPtr fakesink = QGst::ElementFactory::make("fakesink");
    fakesink->setProperty("sync", false);
    fakesink->setProperty("async", false);
    fakesink->setProperty("silent", true);
    fakesink->setProperty("enable-last-sample", false);

    //queue to support fsconference after the tee
    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");

    if (!videoscale || !colorspace || !capsfilter || !tee || !queue || !fakesink) {
        qCWarning(LIBKTPCALL) << "Failed to load basic gstreamer elements";
        return false;
    }

    QGst::BinPtr bin = QGst::Bin::create();
    bin->add(src, videoscale, colorspace, capsfilter, tee, fakesink, queue);

    // src ! (videorate) ! videoscale
    if (videorate) {
        bin->add(videorate);
        if (!QGst::Element::linkMany(src, videorate, videoscale)) {
            qCWarning(LIBKTPCALL) << "Failed to link videosrc ! videorate ! videoscale";
            return false;
        }
    } else {
        qCDebug(LIBKTPCALL) << "NOT using videorate";
        if (!src->link(videoscale)) {
            qCWarning(LIBKTPCALL) << "Failed to link videosrc ! videoscale";
            return false;
        }
    }

    // videoscale ! colorspace ! capsfilter
    if (!QGst::Element::linkMany(videoscale, colorspace, capsfilter)) {
        qCWarning(LIBKTPCALL) << "Failed to link videoscale ! colorspace ! capsfilter";
        return false;
    }

    // capsfilter ! (postproc_tmpnoise) ! tee
    // FIXME: GStreamer 0.10 version used postproc_tmpnoise to reduce video noise
    // there doesn't appear to be an equivalent in gstreamer 1.0.
    // consider investigating alternatives.

    if (!capsfilter->link(tee)) {
        qCWarning(LIBKTPCALL) << "Failed to link capsfilter ! tee";
        return false;
    }

    // tee ! fakesink
    if (tee->getRequestPad("src_%u")->link(fakesink->getStaticPad("sink")) != QGst::PadLinkOk) {
        qCWarning(LIBKTPCALL) << "Failed to link tee ! fakesink";
        return false;
    }

    // tee ! queue
    if (tee->getRequestPad("src_%u")->link(queue->getStaticPad("sink")) != QGst::PadLinkOk) {
        qCWarning(LIBKTPCALL) << "Failed to link tee ! queue";
        return false;
    }

    // create bin's src pad
    bin->addPad(QGst::GhostPad::create(queue->getStaticPad("src"), "src"));

    m_srcBin = bin;
    return true;
}

QGst::CapsPtr TfVideoContentHandler::contentCaps() const
{
    // TfContent advertises the Content.I.VideoControl properties, if the interface exists,
    // otherwise it returns 0 for all of them
    int width = tfContent()->property("width").toInt();
    int height = tfContent()->property("height").toInt();
    if (width == 0 || height == 0) {
        width = 320;
        height = 240;
    }

    int framerate = tfContent()->property("framerate").toInt();
    if (framerate == 0) {
        framerate = 15;
    }

    QGst::Structure capsStruct("video/x-raw");
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
        qCDebug(LIBKTPCALL) << "restarting source with new caps" << caps;

        QString id = tfContent()->property("object-path").toString().section(QLatin1Char('/'), -1);
        QString capsfilterName = QString(QLatin1String("input_capsfilter_%1")).arg(id);
        QGst::ElementPtr capsfilter = m_srcBin->getElementByName(capsfilterName.toLatin1());

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
