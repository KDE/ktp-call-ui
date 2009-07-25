/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#include "farsightmediahandler.h"
#include "audiobin.h"
#include "videoinputbin.h"
#include "../../libkgstdevices/devicemanager.h"
#include "../../libkgstdevices/videowidget.h"
#include "../../libqtgstreamer/qgstpipeline.h"
#include "../../libqtgstreamer/qgstelementfactory.h"
#include "../../libqtgstreamer/qgstglobal.h"
#include "../../libqtgstreamer/qgstbus.h"
#include "../../libqtpfarsight/qtfchannel.h"
#include <KDebug>
#include <KGlobal>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KStandardDirs>

using namespace QtGstreamer;
using namespace KGstDevices;

struct FarsightMediaHandler::Private
{
    Private() : videoWidgetIdsCounter(0) {}

    QTfChannel *qtfchannel;
    DeviceManager *deviceManager;

    QGstPipelinePtr pipeline;
    AudioBinPtr audioInputBin;
    AudioBinPtr audioOutputBin;
    QGstElementPtr audioAdder;
    VideoInputBinPtr videoInputBin;
    QGstElementPtr videoTee;
    QPointer<VideoWidget> videoInputWidget;

    QHash<QByteArray, QGstPadPtr> audioAdderPadsMap;
    QHash<QByteArray, uint> videoWidgetIdsMap;
    uint videoWidgetIdsCounter;
};

FarsightMediaHandler::FarsightMediaHandler(const Tp::StreamedMediaChannelPtr & channel,
                                           QObject *parent)
    : AbstractMediaHandler(parent), d(new Private)
{
    init(channel);
}

FarsightMediaHandler::~FarsightMediaHandler()
{
    delete d;
}

void FarsightMediaHandler::init(const Tp::StreamedMediaChannelPtr & channel)
{
    //initialize gstreamer
    static bool gInitDone = false;
    if ( !gInitDone ) {
        qGstInit();
        gInitDone = true;
    }

    d->pipeline = QGstPipeline::newPipeline();
    d->pipeline->setState(QGstElement::Playing);

    d->qtfchannel = new QTfChannel(channel, d->pipeline->getBus(), this);
    connect(d->qtfchannel, SIGNAL(sessionCreated(QtGstreamer::QGstElementPtr)),
            SLOT(onSessionCreated(QtGstreamer::QGstElementPtr)));
    connect(d->qtfchannel, SIGNAL(closed()), SLOT(onQTfChannelClosed()));

    connect(d->qtfchannel, SIGNAL(openAudioInputDevice(bool*)), SLOT(openAudioInputDevice(bool*)));
    connect(d->qtfchannel, SIGNAL(audioSinkPadAdded(QtGstreamer::QGstPadPtr)),
            SLOT(audioSinkPadAdded(QtGstreamer::QGstPadPtr)));
    connect(d->qtfchannel, SIGNAL(closeAudioInputDevice()), SLOT(closeAudioInputDevice()));

    connect(d->qtfchannel, SIGNAL(openVideoInputDevice(bool*)), SLOT(openVideoInputDevice(bool*)));
    connect(d->qtfchannel, SIGNAL(videoSinkPadAdded(QtGstreamer::QGstPadPtr)),
            SLOT(videoSinkPadAdded(QtGstreamer::QGstPadPtr)));
    connect(d->qtfchannel, SIGNAL(closeVideoInputDevice()), SLOT(closeVideoInputDevice()));

    connect(d->qtfchannel, SIGNAL(openAudioOutputDevice(bool*)), SLOT(openAudioOutputDevice(bool*)));
    connect(d->qtfchannel, SIGNAL(audioSrcPadAdded(QtGstreamer::QGstPadPtr)),
            SLOT(audioSrcPadAdded(QtGstreamer::QGstPadPtr)));
    connect(d->qtfchannel, SIGNAL(audioSrcPadRemoved(QtGstreamer::QGstPadPtr)),
            SLOT(audioSrcPadRemoved(QtGstreamer::QGstPadPtr)));

    connect(d->qtfchannel, SIGNAL(openVideoOutputDevice(bool*)), SLOT(openVideoOutputDevice(bool*)));
    connect(d->qtfchannel, SIGNAL(videoSrcPadAdded(QtGstreamer::QGstPadPtr)),
            SLOT(videoSrcPadAdded(QtGstreamer::QGstPadPtr)));
    connect(d->qtfchannel, SIGNAL(videoSrcPadRemoved(QtGstreamer::QGstPadPtr)),
            SLOT(videoSrcPadRemoved(QtGstreamer::QGstPadPtr)));

    QString codecsFile = KStandardDirs::locate("appdata", "codec-preferences.ini");
    if ( !codecsFile.isEmpty() ) {
        kDebug() << "Reading codec preferences from" << codecsFile;
        d->qtfchannel->setCodecsConfigFile(codecsFile);
    }

    d->deviceManager = new DeviceManager(this);
    d->deviceManager->loadConfig(KGlobal::config()->group("Devices"));
}

void FarsightMediaHandler::onSessionCreated(QGstElementPtr conference)
{
    d->pipeline->add(conference);
    conference->setState(QGstElement::Playing);
}

void FarsightMediaHandler::onQTfChannelClosed()
{
    d->pipeline->setState(QGstElement::Null);
}

void FarsightMediaHandler::openAudioInputDevice(bool *success)
{
    kDebug() << "Opening audio input device";

    QGstElementPtr element = d->deviceManager->newAudioInputElement();
    if ( !element ) {
        emit logMessage(CallLog::Error, i18n("The gstreamer element for the audio input device "
                                             "could not be created. Please check your gstreamer "
                                             "installation"));
        *success = false;
        return;
    }

    if ( !element->setState(QGstElement::Ready) ) {
        emit logMessage(CallLog::Error, i18n("The audio input device could not be initialized"));
        *success = false;
        return;
    }

    d->audioInputBin = AudioBin::createAudioBin(element, AudioBin::Input);
    if ( !d->audioInputBin ) {
        emit logMessage(CallLog::Error, i18n("Some gstreamer elements could not be created. "
                                              "Please check your gstreamer installation."));
        *success = false;
        return;
    }

    d->pipeline->add(d->audioInputBin);
    d->audioInputBin->setState(QGstElement::Playing);
    *success = true;

    emit audioInputDeviceCreated(d->audioInputBin.data());
}

void FarsightMediaHandler::audioSinkPadAdded(QGstPadPtr sinkPad)
{
    kDebug() << "Audio sink pad added";

    Q_ASSERT(!d->audioInputBin.isNull());
    QGstPadPtr srcPad = d->audioInputBin->getStaticPad("src");
    srcPad->link(sinkPad);
}

void FarsightMediaHandler::closeAudioInputDevice()
{
    kDebug() << "Closing audio input device";

    d->pipeline->remove(d->audioInputBin);
    d->audioInputBin->setState(QGstElement::Null);
    d->audioInputBin = AudioBinPtr();
    emit audioInputDeviceDestroyed();
}

void FarsightMediaHandler::openVideoInputDevice(bool *success)
{
    kDebug() << "Opening video input device";

    QGstElementPtr element = d->deviceManager->newVideoInputElement();
    if ( !element ) {
        emit logMessage(CallLog::Error, i18n("The gstreamer element for the video input device "
                                             "could not be created. Please check your gstreamer "
                                             "installation."));
        *success = false;
        return;
    } else if ( !element->setState(QGstElement::Paused) ) {
        emit logMessage(CallLog::Error, i18n("The video input device could not be initialized."));
        *success = false;
        return;
    }

    d->videoInputBin = VideoInputBin::createVideoInputBin(element);
    d->videoTee = QGstElementFactory::make("tee");
    d->videoInputWidget = d->deviceManager->newVideoWidget();

    if ( !d->videoInputBin || !d->videoTee || !d->videoInputWidget ) {
        emit logMessage(CallLog::Error, i18n("Some gstreamer elements could not be created. "
                                              "Please check your gstreamer installation."));
        *success = false;
        return;
    } else if ( !d->videoInputWidget->videoBin()->setState(QGstElement::Ready) ) {
        emit logMessage(CallLog::Error, i18n("The video output driver could not be initialized."));
        *success = false;
        delete d->videoInputWidget;
        return;
    }

    d->pipeline->add(d->videoInputBin);
    d->pipeline->add(d->videoTee);
    d->pipeline->add(d->videoInputWidget->videoBin());

    QGstElement::link(d->videoInputBin, d->videoTee);
    QGstPadPtr srcPad = d->videoTee->getRequestPad("src%d");
    QGstPadPtr sinkPad = d->videoInputWidget->videoBin()->getStaticPad("sink");
    srcPad->link(sinkPad);

    d->videoInputBin->setState(QGstElement::Playing);
    d->videoTee->setState(QGstElement::Playing);
    d->videoInputWidget->videoBin()->setState(QGstElement::Playing);
    *success = true;

    emit videoInputDeviceCreated(d->videoInputBin.data(), d->videoInputWidget);
}

void FarsightMediaHandler::videoSinkPadAdded(QGstPadPtr sinkPad)
{
    kDebug() << "Video sink pad added";

    Q_ASSERT(!d->videoTee.isNull());
    QGstPadPtr srcPad = d->videoTee->getRequestPad("src%d");
    srcPad->link(sinkPad);
}

void FarsightMediaHandler::closeVideoInputDevice()
{
    kDebug() << "Closing video input device";

    d->videoInputBin->setState(QGstElement::Null);
    d->videoTee->setState(QGstElement::Null);
    d->videoInputWidget->videoBin()->setState(QGstElement::Null);

    d->pipeline->remove(d->videoInputBin);
    d->pipeline->remove(d->videoTee);
    d->pipeline->remove(d->videoInputWidget->videoBin());

    d->videoInputBin = VideoInputBinPtr();
    d->videoTee = QGstElementPtr();
    d->videoInputWidget->hide();
    d->videoInputWidget->deleteLater();

    emit videoInputDeviceDestroyed();
}

void FarsightMediaHandler::openAudioOutputDevice(bool *success)
{
    kDebug() << "Opening audio output device";

    QGstElementPtr element = d->deviceManager->newAudioOutputElement();
    if ( !element ) {
        emit logMessage(CallLog::Error, i18n("The gstreamer element for the audio output device "
                                             "could not be created. Please check your gstreamer "
                                             "installation"));
        *success = false;
        return;
    }

    if ( !element->setState(QGstElement::Ready) ) {
        emit logMessage(CallLog::Error, i18n("The audio output device could not be initialized"));
        *success = false;
        return;
    }

    d->audioAdder = QGstElementFactory::make("liveadder");
    if ( !d->audioAdder ) {
        d->audioAdder = QGstElementFactory::make("adder");
    }

    d->audioOutputBin = AudioBin::createAudioBin(element, AudioBin::Output);

    if ( !d->audioOutputBin || !d->audioAdder ) {
        emit logMessage(CallLog::Error, i18n("Some gstreamer elements could not be created. "
                                              "Please check your gstreamer installation."));
        *success = false;
        return;
    }

    d->pipeline->add(d->audioAdder);
    d->pipeline->add(d->audioOutputBin);
    QGstElement::link(d->audioAdder, d->audioOutputBin);
    d->audioAdder->setState(QGstElement::Playing);
    d->audioOutputBin->setState(QGstElement::Playing);
    *success = true;

    emit audioOutputDeviceCreated(d->audioOutputBin.data());
}

void FarsightMediaHandler::audioSrcPadAdded(QGstPadPtr srcPad)
{
    kDebug() << "Audio src pad added";

    Q_ASSERT(!d->audioAdder.isNull() && !d->audioOutputBin.isNull());
    QGstPadPtr sinkPad = d->audioAdder->getRequestPad("sink%d");
    srcPad->link(sinkPad);
    d->audioAdderPadsMap.insert(srcPad->property<QByteArray>("name"), sinkPad);
}

void FarsightMediaHandler::audioSrcPadRemoved(QGstPadPtr srcPad)
{
    kDebug() << "Audio src pad removed";

    QGstPadPtr sinkPad = d->audioAdderPadsMap.take(srcPad->property<QByteArray>("name"));
    srcPad->unlink(sinkPad);
    d->audioAdder->releaseRequestPad(sinkPad);
}

void FarsightMediaHandler::openVideoOutputDevice(bool *success)
{
    kDebug() << "Opening video output device";

    //here we construct a widget just to see if we can, and then destroy it...
    VideoWidget *widget =  d->deviceManager->newVideoWidget();

    if ( !widget ) {
        emit logMessage(CallLog::Error, i18n("Some gstreamer elements could not be created. "
                                              "Please check your gstreamer installation."));
        *success = false;
        return;
    } else if ( !widget->videoBin()->setState(QGstElement::Ready) ) {
        emit logMessage(CallLog::Error, i18n("The video output driver could not be initialized."));
        *success = false;
        delete widget;
        return;
    }

    delete widget;
    *success = true;
}

void FarsightMediaHandler::videoSrcPadAdded(QGstPadPtr srcPad)
{
    kDebug() << "Video src pad added";

    VideoWidget *widget =  d->deviceManager->newVideoWidget();
    Q_ASSERT(widget);

    d->pipeline->add(widget->videoBin());
    widget->videoBin()->setState(QGstElement::Playing);
    QGstPadPtr sinkPad = widget->videoBin()->getStaticPad("sink");
    srcPad->link(sinkPad);

    d->videoWidgetIdsMap.insert(srcPad->property<QByteArray>("name"), d->videoWidgetIdsCounter);
    emit videoOutputWidgetCreated(widget, d->videoWidgetIdsCounter++);
}

void FarsightMediaHandler::videoSrcPadRemoved(QGstPadPtr srcPad)
{
    kDebug() << "Video src pad removed";

    uint id = d->videoWidgetIdsMap.take(srcPad->property<QByteArray>("name"));
    emit closeVideoOutputWidget(id);
}

#include "farsightmediahandler.moc"
