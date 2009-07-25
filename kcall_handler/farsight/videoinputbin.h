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
#ifndef VIDEOINPUTBIN_H
#define VIDEOINPUTBIN_H

#include "../../libqtgstreamer/qgstbin.h"

class VideoInputBin;
typedef QSharedPointer<VideoInputBin> VideoInputBinPtr;

class VideoInputBin : public QtGstreamer::QGstBin
{
    Q_OBJECT
    Q_DISABLE_COPY(VideoInputBin)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness)
    Q_PROPERTY(int contrast READ contrast WRITE setContrast)
    Q_PROPERTY(int hue READ hue WRITE setHue)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation)
public:
    static VideoInputBinPtr createVideoInputBin(const QtGstreamer::QGstElementPtr & elemenet);
    virtual ~VideoInputBin();

    int brightness() const;
    void setBrightness(int brightness);

    int contrast() const;
    void setContrast(int contrast);

    int hue() const;
    void setHue(int hue);

    int saturation() const;
    void setSaturation(int saturation);

protected:
    VideoInputBin();

private:
    struct Private;
    Private *const d;
};

#endif
