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
#ifndef INPUT_CONTROL_BIN_H
#define INPUT_CONTROL_BIN_H

#include <QGst/Bin>
#include <QGst/Pad>

class InputControlBin
{
public:
    InputControlBin(const QGst::ElementPtr & silenceSrc, const QGst::ElementPtr & filter);
    virtual ~InputControlBin();

    QGst::BinPtr bin() const { return m_bin; }

    void connectSource(const QGst::ElementPtr & source);
    void disconnectSource();

    QGst::PadPtr requestSrcPad();
    void releaseSrcPad(const QGst::PadPtr & pad);

private:
    void switchToRealSource();
    void switchToSilenceSource();

    //<ghost src pad, tee request src pad>
    QHash<QGst::PadPtr, QGst::PadPtr> m_pads;
    //the bin that keeps all the elements below grouped together
    QGst::BinPtr m_bin;
    //the tee that gives us the source pads for whomever connects on this bin
    QGst::ElementPtr m_tee;
    //a fake sink that is connected on the tee to avoid not-linked errors
    QGst::ElementPtr m_fakesink;
    //the input-selector that switches between m_source and m_silenceSource
    QGst::ElementPtr m_inputSelector;
    //a fake source that generates silence (audio) or black image (video)
    QGst::ElementPtr m_silenceSource;
    //the real source bin, as constructed from the subclass
    QGst::ElementPtr m_source;
    //the input-selector pad where the source is connected
    QGst::PadPtr m_selectorSinkPad;
};

#endif // INPUT_CONTROL_BIN_H
