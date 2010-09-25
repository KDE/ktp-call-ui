/*
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
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
#ifndef CALLPARTICIPANT_P_H
#define CALLPARTICIPANT_P_H

#include "callparticipant.h"
#include <QtCore/QWeakPointer>
#include <QGst/Pad>
#include <QGst/Pipeline>
#include <QGst/StreamVolume>
#include <QGst/ColorBalance>
#include <QGst/Ui/VideoWidget>
#include <TelepathyQt4/Contact>

class CallParticipantPrivate
{
public:
    inline CallParticipantPrivate(CallParticipant *qq) : q(qq) {}

    void setAudioPads(QGst::PipelinePtr & pipeline,
                      const QGst::PadPtr & srcPad, const QGst::PadPtr & sinkPad);
    void removeAudioPads(QGst::PipelinePtr & pipeline);
    void setVideoPads(QGst::PipelinePtr & pipeline,
                      const QGst::PadPtr & srcPad, const QGst::PadPtr & sinkPad = QGst::PadPtr());
    void removeVideoPads(QGst::PipelinePtr & pipeline);

public:
    QGst::StreamVolumePtr m_volumeControl;
    QGst::ColorBalancePtr m_colorBalanceControl;
    QWeakPointer<QGst::Ui::VideoWidget> m_videoWidget;
    Tp::ContactPtr m_contact;

private:
    CallParticipant *q;

    QGst::BinPtr m_audioBin;
    QGst::BinPtr m_videoBin;
};

#endif
