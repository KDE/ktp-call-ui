/*
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
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
#include "../private/device-element-factory.h"
#include <QtCore/QCoreApplication>
#include <QGst/Init>
#include <QGst/Element>

using namespace KTpCallPrivate;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("configuration_test");

    QGst::init();
    QGst::ElementPtr element = DeviceElementFactory::makeAudioOutputElement();
    if (!element) {
        qDebug() << "Could not make audio output element";
    } else {
        element->setState(QGst::StateNull);
    }

    element = DeviceElementFactory::makeAudioCaptureElement();
    if (!element) {
        qDebug() << "Could not make audio capture element";
    } else {
        element->setState(QGst::StateNull);
    }

    element = DeviceElementFactory::makeVideoCaptureElement();
    if (!element) {
        qDebug() << "Could not make video capture element";
    } else {
        element->setState(QGst::StateNull);
    }
}
