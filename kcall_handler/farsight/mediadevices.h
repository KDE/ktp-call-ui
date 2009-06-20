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
#ifndef _FARSIGHT_MEDIADEVICES_H
#define _FARSIGHT_MEDIADEVICES_H

#include "../abstractmediahandler.h"
#include <gst/gst.h>

namespace Farsight {

class AudioDevice : public AbstractAudioDevice
{
    Q_OBJECT
public:
    static AudioDevice *createInputDevice(QObject *parent = 0);
    static AudioDevice *createOutputDevice(QObject *parent = 0);

    virtual ~AudioDevice();
    GstElement *bin() const;

    virtual QString name() const;
    virtual bool isMuted() const;
    virtual qreal volume() const;

public slots:
    virtual void setMuted(bool mute);
    virtual void setVolume(qreal volume);

protected:
    AudioDevice(const QString & name, QObject *parent = 0);

    GstElement *m_bin;
    GstElement *m_volumeElement;
    QString m_name;
};

} //namespace Farsight

#endif
