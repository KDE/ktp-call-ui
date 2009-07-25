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
#ifndef ABSTRACTMEDIAHANDLER_H
#define ABSTRACTMEDIAHANDLER_H

#include "calllog.h"
#include <TelepathyQt4/StreamedMediaChannel>

class AbstractMediaHandler : public QObject
{
    Q_OBJECT
public:
    static AbstractMediaHandler *create(const Tp::StreamedMediaChannelPtr & channel,
                                        QObject *parent = 0);

Q_SIGNALS:
    void audioInputDeviceCreated(QObject *inputVolumeControl);
    void audioInputDeviceDestroyed();

    void audioOutputDeviceCreated(QObject *outputVolumeControl);
    void audioOutputDeviceDestroyed();

    void videoInputDeviceCreated(QWidget *videoWidget);
    void videoInputDeviceDestroyed();

    void videoOutputWidgetCreated(QWidget *videoWidget, uint streamId);
    void closeVideoOutputWidget(uint streamId);

    void logMessage(CallLog::LogType logType, const QString & message);

protected:
    AbstractMediaHandler(QObject *parent = 0);
};

#endif
