/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
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
#ifndef VOLUME_CONTROLLER_H
#define VOLUME_CONTROLLER_H

#include <QtCore/QObject>
#include <QGst/StreamVolume>

class VolumeController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool volumeControlSupported READ volumeControlSupported
                                           NOTIFY volumeControlSupportedChanged)
    Q_PROPERTY(uint volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
public:
    VolumeController(QObject *parent = 0);
    virtual ~VolumeController();

    bool volumeControlSupported() const;
    uint volume() const;
    bool isMuted() const;

    QGst::StreamVolumePtr element() const;
    void setElement(const QGst::StreamVolumePtr & streamVolumeElement);

public Q_SLOTS:
    void setVolume(uint volume);
    void setMuted(bool muted);

Q_SIGNALS:
    void volumeControlSupportedChanged(bool isSupported);
    void volumeChanged(uint newVolume);
    void mutedChanged(bool isMuted);

private:
    QGst::StreamVolumePtr m_volumeControl;
};

#endif // VOLUME_CONTROLLER_H
