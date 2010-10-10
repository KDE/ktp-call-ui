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
#include "callparticipant_p.h"
#include <QGst/ElementFactory>
#include <QGst/GhostPad>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/Connection>

CallParticipant::CallParticipant(const Tp::ContactPtr & contact, QObject *parent)
    : QObject(parent), d(new CallParticipantPrivate(this))
{
    d->m_contact = contact;
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
    delete d;
}

Tp::ContactPtr CallParticipant::contact() const
{
    return d->m_contact;
}

bool CallParticipant::isMyself() const
{
    return d->m_contact == d->m_contact->manager()->connection()->selfContact();
}

bool CallParticipant::hasAudioStream() const
{
    return !d->m_volumeControl.isNull();
}

bool CallParticipant::hasVideoStream() const
{
    return !d->m_colorBalanceControl.isNull();
}

bool CallParticipant::isMuted() const
{
    return d->m_volumeControl ? d->m_volumeControl->isMuted() : true;
}

void CallParticipant::setMuted(bool mute)
{
    if (d->m_volumeControl) {
        d->m_volumeControl->setMuted(mute);
    }
}

//TODO manual says Cubic scale is more appropriate for GUIs
int CallParticipant::volume() const
{
    return d->m_volumeControl ? static_cast<int>(d->m_volumeControl->volume() * 100) : 0;
}

void CallParticipant::setVolume(int volume)
{
    if (d->m_volumeControl) {
        d->m_volumeControl->setVolume(double(qBound<int>(0, volume, 1000)) / double(100.0));
    }
}

QWidget *CallParticipant::videoWidget() const
{
    return d->m_videoWidget.data();
}

//TODO use the GstColorBalance interface properly, instead of hardcoding min/max values, etc...
int CallParticipant::brightness() const
{
    return d->m_colorBalanceControl ?
        static_cast<int>((d->m_colorBalanceControl->property("brightness").get<double>() * 100)) : 0;
}

void CallParticipant::setBrightness(int brightness)
{
    if (d->m_colorBalanceControl) {
        brightness = qBound(-100, brightness, 100);
        d->m_colorBalanceControl->setProperty("brightness", double(brightness) / double(100));
    }
}

int CallParticipant::contrast() const
{
    return d->m_colorBalanceControl ?
        static_cast<int>((d->m_colorBalanceControl->property("contrast").get<double>() * 100)) : 0;
}

void CallParticipant::setContrast(int contrast)
{
    if (d->m_colorBalanceControl) {
        contrast = qBound(0, contrast, 200);
        d->m_colorBalanceControl->setProperty("contrast", double(contrast) / double(100));
    }
}

int CallParticipant::hue() const
{
    return d->m_colorBalanceControl ?
        static_cast<int>((d->m_colorBalanceControl->property("hue").get<double>() * 100)) : 0;
}

void CallParticipant::setHue(int hue)
{
    if (d->m_colorBalanceControl) {
        hue = qBound(-100, hue, 100);
        d->m_colorBalanceControl->setProperty("hue", double(hue) / double(100));
    }
}

int CallParticipant::saturation() const
{
    return d->m_colorBalanceControl ?
        static_cast<int>((d->m_colorBalanceControl->property("saturation").get<double>() * 100)) : 0;
}

void CallParticipant::setSaturation(int saturation)
{
    if (d->m_colorBalanceControl) {
        saturation = qBound(0, saturation, 200);
        d->m_colorBalanceControl->setProperty("saturation", double(saturation) / double(100));
    }
}

void CallParticipantPrivate::setAudioPads(QGst::PipelinePtr & pipeline,
                                          QGst::PadPtr & srcPad, const QGst::PadPtr & sinkPad)
{
    m_audioBin = QGst::Bin::create();

    m_volumeControl = QGst::ElementFactory::make("volume").dynamicCast<QGst::StreamVolume>();
    m_audioBin->add(m_volumeControl);
    m_audioBin->addPad(QGst::GhostPad::create(m_volumeControl->getStaticPad("sink"), "sink"));

    //tee is there to support future stream capture capabilities
    QGst::ElementPtr tee = QGst::ElementFactory::make("tee");
    m_audioBin->add(tee);
    m_volumeControl->link(tee);

    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");
    m_audioBin->add(queue);
    tee->getRequestPad("src%d")->link(queue->getStaticPad("sink"));

    QGst::ElementPtr audioConvert = QGst::ElementFactory::make("audioconvert");
    m_audioBin->add(audioConvert);
    queue->link(audioConvert);

    QGst::ElementPtr audioResample = QGst::ElementFactory::make("audioresample");
    m_audioBin->add(audioResample);
    audioConvert->link(audioResample);

    m_audioBin->addPad(QGst::GhostPad::create(audioResample->getStaticPad("src"), "src"));

    pipeline->add(m_audioBin);
    m_audioBin->getStaticPad("src")->link(sinkPad);
    srcPad->link(m_audioBin->getStaticPad("sink"));
    m_audioBin->setState(QGst::StatePlaying);

    Q_EMIT q->audioStreamAdded(q);
}

void CallParticipantPrivate::removeAudioPads(QGst::PipelinePtr & pipeline)
{
    m_volumeControl = QGst::StreamVolumePtr(); //drop reference
    m_audioBin->setState(QGst::StateNull);
    pipeline->remove(m_audioBin);
    m_audioBin = QGst::BinPtr(); //drop reference and destroy the bin and its contents

    Q_EMIT q->audioStreamRemoved(q);
}

void CallParticipantPrivate::setVideoPads(QGst::PipelinePtr & pipeline,
                                          QGst::PadPtr & srcPad, const QGst::PadPtr & sinkPad)
{
    m_videoBin = QGst::Bin::create();

    m_colorBalanceControl = QGst::ElementFactory::make("videobalance").dynamicCast<QGst::ColorBalance>();
    m_videoBin->add(m_colorBalanceControl);
    m_videoBin->addPad(QGst::GhostPad::create(m_colorBalanceControl->getStaticPad("sink"), "sink"));

    QGst::ElementPtr tee = QGst::ElementFactory::make("tee");
    m_videoBin->add(tee);
    m_colorBalanceControl->link(tee);

    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");
    m_videoBin->add(queue);
    tee->getRequestPad("src%d")->link(queue->getStaticPad("sink"));

    QGst::ElementPtr colorspace = QGst::ElementFactory::make("ffmpegcolorspace");
    m_videoBin->add(colorspace);
    queue->link(colorspace);

    QGst::ElementPtr videoscale = QGst::ElementFactory::make("videoscale");
    m_videoBin->add(videoscale);
    colorspace->link(videoscale);

    QGst::ElementPtr videosink = QGst::ElementFactory::make("qwidgetvideosink"); //FIXME not always the best sink
    videosink->setProperty("force-aspect-ratio", true); //FIXME should be externally configurable
    m_videoBin->add(videosink);
    videoscale->link(videosink);

    m_videoWidget = new QGst::Ui::VideoWidget;
    m_videoWidget.data()->setVideoSink(videosink);

    if (!sinkPad.isNull()) {
        QGst::ElementPtr queue2 = QGst::ElementFactory::make("queue");
        m_videoBin->add(queue2);
        tee->getRequestPad("src%d")->link(queue2->getStaticPad("sink"));

        m_videoBin->addPad(QGst::GhostPad::create(queue2->getStaticPad("src"), "src"));
    }

    pipeline->add(m_videoBin);
    if (!sinkPad.isNull()) {
        m_videoBin->getStaticPad("src")->link(sinkPad);
    }
    srcPad->link(m_videoBin->getStaticPad("sink"));
    m_videoBin->setState(QGst::StatePlaying);

    Q_EMIT q->videoStreamAdded(q);
}

void CallParticipantPrivate::removeVideoPads(QGst::PipelinePtr & pipeline)
{
    m_colorBalanceControl = QGst::ColorBalancePtr(); //drop reference
    m_videoWidget.data()->setVideoSink(QGst::ElementPtr()); //drop the widget's reference to the video sink

    m_videoBin->setState(QGst::StateNull);
    pipeline->remove(m_videoBin);
    m_videoBin = QGst::BinPtr(); //drop reference and destroy the bin and its contents

    Q_EMIT q->videoStreamRemoved(q);
}

#include "callparticipant.moc"
