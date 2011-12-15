/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.com>

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
#include "input-control-bin.h"

#include <QtCore/QString>

#include <QGst/ElementFactory>
#include <QGst/GhostPad>
#include <QGst/Structure>
#include <QGst/Caps>
#include <QGst/Fraction>


InputControlBin::InputControlBin(const QGst::ElementPtr & silenceSrc,
                                 const QGst::ElementPtr & filter)
{
    m_bin = QGst::Bin::create();
    m_silenceSource = silenceSrc;
    m_inputSelector = QGst::ElementFactory::make("input-selector");
    m_tee = QGst::ElementFactory::make("tee");
    m_fakesink = QGst::ElementFactory::make("fakesink");
    m_fakesink->setProperty("sync", false);
    m_fakesink->setProperty("async", false);
    m_fakesink->setProperty("silent", true);
    m_fakesink->setProperty("enable-last-buffer", false);

    m_bin->add(m_silenceSource, m_inputSelector, filter, m_tee, m_fakesink);

    // silencesource ! input-selector
    QGst::PadPtr selectorSinkPad = m_inputSelector->getRequestPad("sink%d");
    m_silenceSource->getStaticPad("src")->link(selectorSinkPad);

    // input-selector ! (filter bin) ! tee
    QGst::Element::linkMany(m_inputSelector, filter, m_tee);

    // tee ! fakesink
    m_tee->getRequestPad("src%d")->link(m_fakesink->getStaticPad("sink"));
}

InputControlBin::~InputControlBin()
{

}

void InputControlBin::connectSource(const QGst::ElementPtr & source)
{
    m_source = source;
    m_bin->add(m_source);

    //connect the source to the input-selector sink pad
    m_selectorSinkPad = m_inputSelector->getRequestPad("sink%d");
    m_source->getStaticPad("src")->link(m_selectorSinkPad);

    //set the source to playing state
    m_source->syncStateWithParent();

    //swap sources
    switchToRealSource();
}

void InputControlBin::disconnectSource()
{
    switchToSilenceSource();

    //disconnect the source from the input-selector sink pad
    m_source->getStaticPad("src")->unlink(m_selectorSinkPad);
    m_inputSelector->releaseRequestPad(m_selectorSinkPad);
    m_selectorSinkPad.clear();

    //destroy the source
    m_source->setState(QGst::StateNull);
    m_bin->remove(m_source);
    m_source.clear();
}

QGst::PadPtr InputControlBin::requestSrcPad()
{
    static uint padNameCounter = 0;

    QString newPadName = QString("src%1").arg(padNameCounter);
    padNameCounter++;

    QGst::PadPtr teeSrcPad = m_tee->getRequestPad("src%d");
    QGst::PadPtr ghostSrcPad = QGst::GhostPad::create(teeSrcPad, newPadName.toAscii());

    ghostSrcPad->setActive(true);
    m_bin->addPad(ghostSrcPad);
    m_pads.insert(ghostSrcPad, teeSrcPad);

    return ghostSrcPad;
}

void InputControlBin::releaseSrcPad(const QGst::PadPtr & pad)
{
    QGst::PadPtr teeSrcPad = m_pads.take(pad);
    Q_ASSERT(!teeSrcPad.isNull());

    m_bin->removePad(pad);
    m_tee->releaseRequestPad(teeSrcPad);
}

void InputControlBin::switchToRealSource()
{
    //wait for data to arrive on the source src pad
    //required for synchronization with the streaming thread
    QGst::PadPtr sourceSrcPad = m_source->getStaticPad("src");
    sourceSrcPad->setBlocked(true);
    sourceSrcPad->setBlocked(false);

    //switch inputs
    m_inputSelector->setProperty("active-pad", m_selectorSinkPad);
}

void InputControlBin::switchToSilenceSource()
{
    //switch inputs
    m_inputSelector->setProperty("active-pad", m_inputSelector->getStaticPad("sink0"));

    //wait for data to arrive on the inputselector src pad
    //required for synchronization with the streaming thread
    QGst::PadPtr selectorSrcPad = m_inputSelector->getStaticPad("src");
    selectorSrcPad->setBlocked(true);
    selectorSrcPad->setBlocked(false);
}

