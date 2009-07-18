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
#ifndef FARSIGHTMEDIAHANDLER_H
#define FARSIGHTMEDIAHANDLER_H

#include "../abstractmediahandler.h"
#include "../../libqtgstreamer/qgstelement.h"

class FarsightMediaHandler : public AbstractMediaHandler
{
    Q_OBJECT
public:
    explicit FarsightMediaHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent = 0);
    virtual ~FarsightMediaHandler();

private:
    void init(const Tp::StreamedMediaChannelPtr & channel);

private slots:
    void onSessionCreated(QtGstreamer::QGstElementPtr conference);
    void onQTfChannelClosed();

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
    struct Private;
    Private *const d;
};

#endif
