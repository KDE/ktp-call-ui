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
#include "../private/source-controllers.h"
#include <QGst/Init>
#include <QGst/Pipeline>
#include <QGst/Pad>
#include <QGst/ElementFactory>
#include <QtCore/QCoreApplication>
#include <KDebug>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QGst::init(&argc, &argv);

    QGst::PipelinePtr pipeline = QGst::Pipeline::create();

    QGst::ElementPtr sink = QGst::ElementFactory::make("autoaudiosink");
    sink->setState(QGst::StateReady);

    pipeline->add(sink);
    pipeline->setState(QGst::StatePlaying);

    QGst::PadPtr sinkPad = sink->getStaticPad("sink");
    QElapsedTimer timer;
    AudioSourceController *sc = new AudioSourceController(pipeline);

    kDebug() << "adding...";
    timer.start();
    QGst::PadPtr srcPad = sc->requestSrcPad();
    srcPad->link(sinkPad);
    kDebug() << "added" << timer.elapsed();

    sleep(3);

    kDebug() << "connecting...";
    timer.restart();
    sc->connectToSource();
    kDebug() << "connected" << timer.elapsed();

    sleep(3);

    kDebug() << "disconnecting...";
    timer.restart();
    sc->disconnectFromSource();
    kDebug() << "disconnected" << timer.elapsed();

    sleep(3);

    kDebug() << "connecting...";
    timer.restart();
    sc->connectToSource();
    kDebug() << "connected" << timer.elapsed();

    sleep(3);

    kDebug() << "disconnecting...";
    timer.restart();
    sc->disconnectFromSource();
    kDebug() << "disconnected" << timer.elapsed();

    kDebug() << "removing...";
    timer.restart();
    srcPad->unlink(sinkPad);
    sc->releaseSrcPad(srcPad);
    kDebug() << "removed" << timer.elapsed();

    delete sc;
    pipeline->setState(QGst::StateNull);

    return 0;
}
