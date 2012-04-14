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
#include "volume-controller.h"

VolumeController::VolumeController(QObject *parent)
    : QObject(parent)
{
}

VolumeController::~VolumeController()
{
}

bool VolumeController::volumeControlSupported() const
{
    return m_volumeControl;
}

uint VolumeController::volume() const
{
    return m_volumeControl ? m_volumeControl->volume(QGst::StreamVolumeFormatCubic)*1000U : 0U;
}

void VolumeController::setVolume(uint vol)
{
    if (m_volumeControl && volume() != vol) {
        m_volumeControl->setVolume(qBound(0U, vol, 1000U)/1000.0, QGst::StreamVolumeFormatCubic);
        Q_EMIT volumeChanged(volume());
    }
}

bool VolumeController::isMuted() const
{
    return m_volumeControl ? m_volumeControl->isMuted() : true;
}

void VolumeController::setMuted(bool muted)
{
    if (m_volumeControl && isMuted() != muted) {
        m_volumeControl->setMuted(muted);
        Q_EMIT mutedChanged(isMuted());
    }
}

QGst::StreamVolumePtr VolumeController::element() const
{
    return m_volumeControl;
}

void VolumeController::setElement(const QGst::StreamVolumePtr & streamVolumeElement)
{
    bool emitChanged = (!m_volumeControl && streamVolumeElement)
                       || (m_volumeControl && !streamVolumeElement);

    m_volumeControl = streamVolumeElement;

    if (emitChanged) {
        Q_EMIT volumeControlSupportedChanged(volumeControlSupported());
        Q_EMIT mutedChanged(isMuted());
        Q_EMIT volumeChanged(volume());
    }
}

#include "moc_volume-controller.cpp"
