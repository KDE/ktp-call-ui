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
#ifndef _FARSIGHT_MEDIAHANDLER_H
#define _FARSIGHT_MEDIAHANDLER_H

#include "../abstractmediahandler.h"
typedef struct _GstPad GstPad;

namespace Farsight {

class MediaHandler : public AbstractMediaHandler
{
    Q_OBJECT
public:
    explicit MediaHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent = 0);
    virtual ~MediaHandler();

private:
    void initialize();
    void stop();
    bool createAudioOutputDevice();
    Q_INVOKABLE void onAudioSrcPadAdded(GstPad *srcPad);
    void onAudioSinkPadAdded(GstPad *sinkPad);
    Q_INVOKABLE void onVideoSrcPadAdded(GstPad *srcPad, uint streamId);

    struct Private;
    friend class Private;
    Private *const d;
};

} //namespace Farsight

#endif
