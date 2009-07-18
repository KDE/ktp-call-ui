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
#ifndef AUDIOBIN_H
#define AUDIOBIN_H

#include "../../libqtgstreamer/qgstbin.h"

class AudioBin;
typedef QSharedPointer<AudioBin> AudioBinPtr;

class AudioBin : public QtGstreamer::QGstBin
{
    Q_OBJECT
    Q_DISABLE_COPY(AudioBin)
    Q_PROPERTY(bool mute READ isMuted WRITE setMuted)
    Q_PROPERTY(int minVolume READ minVolume)
    Q_PROPERTY(int maxVolume READ maxVolume)
    Q_PROPERTY(int volume READ volume WRITE setVolume)
public:
    enum Type { Input, Output };
    static AudioBinPtr createAudioBin(const QtGstreamer::QGstElementPtr & element, Type type);

    virtual ~AudioBin();

    bool isMuted() const;
    void setMuted(bool mute);

    int minVolume() const;
    int maxVolume() const;
    int volume() const;
    void setVolume(int volume);

protected:
    AudioBin();

private:
    struct Private;
    Private *const d;
};

#endif
