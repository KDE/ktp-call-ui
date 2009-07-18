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
#include "audiobin.h"
#include "../../libqtgstreamer/qgstelementfactory.h"
#include "../../libqtgstreamer/qgstghostpad.h"

using namespace QtGstreamer;

struct AudioBin::Private
{
    QGstElementPtr volume;
};

//static
AudioBinPtr AudioBin::createAudioBin(const QGstElementPtr & element, AudioBin::Type type)
{
    QGstElementPtr volume = QGstElementFactory::make("volume");
    QGstElementPtr audioconvert = QGstElementFactory::make("audioconvert");
    QGstElementPtr audioresample = QGstElementFactory::make("audioresample");
    if ( !volume || !audioconvert || !audioresample ) {
        return AudioBinPtr();
    }

    AudioBin *bin = new AudioBin;
    bin->add(element);
    bin->add(volume);
    bin->add(audioconvert);
    bin->add(audioresample);
    bin->d->volume = volume;

    if ( type == Output ) {
        QGstElement::link(audioconvert, audioresample, volume, element);
        QGstPadPtr sink = audioconvert->getStaticPad("sink");
        QGstPadPtr ghost = QGstGhostPad::newGhostPad("sink", sink);
        bin->addPad(ghost);
    } else {
        QGstElement::link(element, volume, audioconvert, audioresample);
        QGstPadPtr src = audioresample->getStaticPad("src");
        QGstPadPtr ghost = QGstGhostPad::newGhostPad("src", src);
        bin->addPad(ghost);
    }
    return AudioBinPtr(bin);
}

AudioBin::AudioBin()
    : QGstBin(), d(new Private)
{
}

AudioBin::~AudioBin()
{
    delete d;
}

bool AudioBin::isMuted() const
{
    return d->volume->property<bool>("mute");
}

void AudioBin::setMuted(bool mute)
{
    d->volume->setProperty("mute", mute);
}

int AudioBin::minVolume() const
{
    return 0;
}

int AudioBin::maxVolume() const
{
    return 1000;
}

int AudioBin::volume() const
{
    return static_cast<int>(d->volume->property<double>("volume")*100);
}

void AudioBin::setVolume(int volume)
{
    d->volume->setProperty("volume", double(qBound<int>(0, volume, 1000)) / double(100.0));
}

#include "audiobin.moc"
