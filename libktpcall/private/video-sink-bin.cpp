/*
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

#include "video-sink-bin.h"
#include <QGst/ElementFactory>
#include <QGst/GhostPad>

VideoSinkBin::VideoSinkBin(const QGst::ElementPtr & videoSink)
{
    m_bin = QGst::Bin::create();

    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");
    QGst::ElementPtr colorspace = QGst::ElementFactory::make("ffmpegcolorspace");

    m_bin->add(queue, colorspace, videoSink);

    queue->link(colorspace);
    colorspace->link(videoSink);

    QGst::PadPtr sinkPad = queue->getStaticPad("sink");
    QGst::PadPtr sinkGhostPad = QGst::GhostPad::create(sinkPad, "sink");
    m_bin->addPad(sinkGhostPad);
}

VideoSinkBin::~VideoSinkBin()
{
}

