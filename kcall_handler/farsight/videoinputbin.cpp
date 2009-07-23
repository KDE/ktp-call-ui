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

//static
VideoInputBinPtr VideoInputBin::createVideoInputBin(const QGstElementPtr & element)
{
    QGstElementPtr colorSpace = QGstElementFactory::make("ffmpegcolorspace");
    QGstElementPtr videoScale = QGstElementFactory::make("videoscale");
    QGstElementPtr videoRate = QGstElementFactory::make("videorate");
    QGstElementPtr capsFilter = QGstElementFactory::make("capsfilter");
    if ( !colorSpace || !videoScale || !videoRate || !capsFilter ) {
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
    bin->add(capsFilter);
    QGstElement::link(element, colorSpace, videoScale, videoRate, capsFilter);

    QGstPadPtr src = capsFilter->getStaticPad("src");
    QGstPadPtr ghost = QGstGhostPad::newGhostPad("src", src);
    bin->addPad(ghost);

    return VideoInputBinPtr(bin);
}

VideoInputBin::VideoInputBin()
{
}

VideoInputBin::~VideoInputBin()
{
}

#include "videoinputbin.moc"
