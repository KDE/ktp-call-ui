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

namespace Farsight {

class MediaHandler : public AbstractMediaHandler
{
    Q_OBJECT
public:
    MediaHandler(const Tp::StreamedMediaChannelPtr & channel, QObject *parent = 0);
    virtual ~MediaHandler();

    virtual Capabilities capabilities() const;
    virtual AbstractAudioDevice *audioInputDevice() const;
    virtual AbstractAudioDevice *audioOutputDevice() const;

private:
    void initialize();
    void stop();

    struct Private;
    friend class Private;
    Private *const d;
};

} //namespace Farsight

#endif