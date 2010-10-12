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
#include "callchannelhandler_p.h"
#include <QGst/ElementFactory>
#include <QGst/GhostPad>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/Connection>

CallParticipant::CallParticipant(const QExplicitlySharedDataPointer<ParticipantData> & dd, QObject *parent)
    : QObject(parent), d(dd)
{
    setProperty("minVolume", 0);
    setProperty("maxVolume", 1000);
    setProperty("minBrightness", -100);
    setProperty("maxBrightness", 100);
    setProperty("minContrast", 0);
    setProperty("maxContrast", 200);
    setProperty("minHue", -100);
    setProperty("maxHue", 100);
    setProperty("minSaturation", 0);
    setProperty("maxSaturation", 200);
}

CallParticipant::~CallParticipant()
{
}

Tp::ContactPtr CallParticipant::contact() const
{
    return Tp::ContactPtr(); //TODO
}

bool CallParticipant::isMyself() const
{
    return d->who == CallChannelHandlerPrivate::Myself;
}

bool CallParticipant::hasAudioStream() const
{
    return !d->volumeControl.isNull();
}

bool CallParticipant::hasVideoStream() const
{
    return !d->colorBalanceControl.isNull();
}

bool CallParticipant::isMuted() const
{
    return d->volumeControl ? d->volumeControl->isMuted() : true;
}

void CallParticipant::setMuted(bool mute)
{
    if (d->volumeControl) {
        d->volumeControl->setMuted(mute);
    }
}

//TODO manual says Cubic scale is more appropriate for GUIs
int CallParticipant::volume() const
{
    return d->volumeControl ? static_cast<int>(d->volumeControl->volume() * 100) : 0;
}

void CallParticipant::setVolume(int volume)
{
    if (d->volumeControl) {
        d->volumeControl->setVolume(double(qBound<int>(0, volume, 1000)) / double(100.0));
    }
}

QWidget *CallParticipant::videoWidget() const
{
    return d->videoWidget.data();
}

//TODO use the GstColorBalance interface properly, instead of hardcoding min/max values, etc...
int CallParticipant::brightness() const
{
    return d->colorBalanceControl ?
        static_cast<int>((d->colorBalanceControl->property("brightness").get<double>() * 100)) : 0;
}

void CallParticipant::setBrightness(int brightness)
{
    if (d->colorBalanceControl) {
        brightness = qBound(-100, brightness, 100);
        d->colorBalanceControl->setProperty("brightness", double(brightness) / double(100));
    }
}

int CallParticipant::contrast() const
{
    return d->colorBalanceControl ?
        static_cast<int>((d->colorBalanceControl->property("contrast").get<double>() * 100)) : 0;
}

void CallParticipant::setContrast(int contrast)
{
    if (d->colorBalanceControl) {
        contrast = qBound(0, contrast, 200);
        d->colorBalanceControl->setProperty("contrast", double(contrast) / double(100));
    }
}

int CallParticipant::hue() const
{
    return d->colorBalanceControl ?
        static_cast<int>((d->colorBalanceControl->property("hue").get<double>() * 100)) : 0;
}

void CallParticipant::setHue(int hue)
{
    if (d->colorBalanceControl) {
        hue = qBound(-100, hue, 100);
        d->colorBalanceControl->setProperty("hue", double(hue) / double(100));
    }
}

int CallParticipant::saturation() const
{
    return d->colorBalanceControl ?
        static_cast<int>((d->colorBalanceControl->property("saturation").get<double>() * 100)) : 0;
}

void CallParticipant::setSaturation(int saturation)
{
    if (d->colorBalanceControl) {
        saturation = qBound(0, saturation, 200);
        d->colorBalanceControl->setProperty("saturation", double(saturation) / double(100));
    }
}

#include "callparticipant.moc"
