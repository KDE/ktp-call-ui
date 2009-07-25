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
#include "videoinputbin.h"
#include "../../libqtgstreamer/qgstelementfactory.h"
#include "../../libqtgstreamer/qgstghostpad.h"
#include "../../libqtgstreamer/qgstcaps.h"
#include "../../libqtgstreamer/qgststructure.h"

using namespace QtGstreamer;

struct VideoInputBin::Private
{
    QGstElementPtr videoBalance;
};

//static
VideoInputBinPtr VideoInputBin::createVideoInputBin(const QGstElementPtr & element)
{
    QGstElementPtr colorSpace = QGstElementFactory::make("ffmpegcolorspace");
    QGstElementPtr videoScale = QGstElementFactory::make("videoscale");
    QGstElementPtr videoRate = QGstElementFactory::make("videorate");
    QGstElementPtr videoBalance = QGstElementFactory::make("videobalance");
    QGstElementPtr capsFilter = QGstElementFactory::make("capsfilter");
    if ( !colorSpace || !videoScale || !videoRate || !videoBalance || !capsFilter ) {
        return VideoInputBinPtr();
    }

    QGstCapsPtr caps = QGstCaps::newEmpty();
    caps->makeWritable();
    QGstStructure s("video/x-raw-yuv");
    s.setValue("width", 320);
    s.setValue("height", 240);
    caps->appendStructure(s);
    capsFilter->setProperty("caps", caps);

    VideoInputBin *bin = new VideoInputBin;
    bin->add(element);
    bin->add(colorSpace);
    bin->add(videoScale);
    bin->add(videoRate);
    bin->add(videoBalance);
    bin->add(capsFilter);
    QGstElement::link(element, colorSpace, videoScale, videoRate, videoBalance, capsFilter);

    QGstPadPtr src = capsFilter->getStaticPad("src");
    QGstPadPtr ghost = QGstGhostPad::newGhostPad("src", src);
    bin->addPad(ghost);

    bin->d->videoBalance = videoBalance;
    return VideoInputBinPtr(bin);
}

VideoInputBin::VideoInputBin()
    : QGstBin(), d(new Private)
{
    // QObject:: is needed because QGstObject also has a setProperty function
    QObject::setProperty("minBrightness", -100);
    QObject::setProperty("maxBrightness", 100);
    QObject::setProperty("minContrast", 0);
    QObject::setProperty("maxContrast", 200);
    QObject::setProperty("minHue", -100);
    QObject::setProperty("maxHue", 100);
    QObject::setProperty("minSaturation", 0);
    QObject::setProperty("maxSaturation", 200);
}

VideoInputBin::~VideoInputBin()
{
    delete d;
}

int VideoInputBin::brightness() const
{
    return static_cast<int>((d->videoBalance->property<qreal>("brightness") * 100));
}

void VideoInputBin::setBrightness(int brightness)
{
    brightness = qBound(-100, brightness, 100);
    d->videoBalance->setProperty("brightness", qreal(brightness) / qreal(100));
}

int VideoInputBin::contrast() const
{
    return static_cast<int>((d->videoBalance->property<qreal>("contrast") * 100));
}

void VideoInputBin::setContrast(int contrast)
{
    contrast = qBound(0, contrast, 200);
    d->videoBalance->setProperty("contrast", qreal(contrast) / qreal(100));
}

int VideoInputBin::hue() const
{
    return static_cast<int>((d->videoBalance->property<qreal>("hue") * 100));
}

void VideoInputBin::setHue(int hue)
{
    hue = qBound(-100, hue, 100);
    d->videoBalance->setProperty("hue", qreal(hue) / qreal(100));
}

int VideoInputBin::saturation() const
{
    return static_cast<int>((d->videoBalance->property<qreal>("saturation") * 100));
}

void VideoInputBin::setSaturation(int saturation)
{
    saturation = qBound(0, saturation, 200);
    d->videoBalance->setProperty("saturation", qreal(saturation) / qreal(100));
}

#include "videoinputbin.moc"
