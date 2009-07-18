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
#ifndef QTFCHANNEL_H
#define QTFCHANNEL_H

#include "../libqtgstreamer/qgstpad.h"
#include "../libqtgstreamer/qgstelement.h"
#include <TelepathyQt4/StreamedMediaChannel>

/** This is a wrapper class around TfChannel and TfStream that provides a saner Qt-ish API */
class QTfChannel : public QObject
{
    Q_OBJECT
public:
    QTfChannel(Tp::StreamedMediaChannelPtr channel, QtGstreamer::QGstBusPtr bus, QObject *parent = 0);
    virtual ~QTfChannel();

    void setCodecsConfigFile(const QString & filename);
    void setGstBus(QtGstreamer::QGstBusPtr bus);

Q_SIGNALS:
    void sessionCreated(QtGstreamer::QGstElementPtr conference);
    void closed();

    void openAudioInputDevice(bool *success);
    void audioSinkPadAdded(QtGstreamer::QGstPadPtr sinkPad);
    void closeAudioInputDevice();

    void openVideoInputDevice(bool *success);
    void videoSinkPadAdded(QtGstreamer::QGstPadPtr sinkPad);
    void closeVideoInputDevice();

    void openAudioOutputDevice(bool *success);
    void audioSrcPadAdded(QtGstreamer::QGstPadPtr srcPad);
    void audioSrcPadRemoved(QtGstreamer::QGstPadPtr srcPad);

    void openVideoOutputDevice(bool *success);
    void videoSrcPadAdded(QtGstreamer::QGstPadPtr srcPad);
    void videoSrcPadRemoved(QtGstreamer::QGstPadPtr srcPad);

private:
    class Private;
    friend class Private;
    Private *const d;
    Q_PRIVATE_SLOT(d, void onBusMessage(GstMessage *message));
};

#endif
