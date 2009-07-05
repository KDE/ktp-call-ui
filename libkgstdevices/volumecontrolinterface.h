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
#ifndef VOLUMECONTROLINTERFACE_H
#define VOLUMECONTROLINTERFACE_H

class VolumeControlInterface
{
public:
    virtual ~VolumeControlInterface() {}

    virtual bool volumeControlIsAvailable() const = 0;

    virtual bool isMuted() const = 0;
    virtual void setMuted(bool mute) = 0;

    virtual int minVolume() const = 0;
    virtual int maxVolume() const = 0;
    virtual int volume() const = 0;
    virtual void setVolume(int volume) = 0;
};

#endif
