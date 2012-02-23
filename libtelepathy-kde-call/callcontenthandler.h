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
#ifndef CALLCONTENTHANDLER_H
#define CALLCONTENTHANDLER_H

#include "sourcecontrollers.h"
#include "sinkcontrollers.h"
#include <TelepathyQt/CallContent>

class CallContentHandlerPrivate;
class PendingCallContentHandler;
class CallChannelHandlerPrivate;

/** This class handles streaming in a telepathy Call channel Content.
 * Everything related to streaming is handled internally. You only need
 * to access the controller classes that this class exports to be able
 * to control properties of the internal gstreamer pipeline.
 */
class CallContentHandler : public QObject
{
    Q_OBJECT
public:
    /** \returns the telepathy content associated with this handler */
    Tp::CallContentPtr callContent() const;

    /** \returns the controller for the input source that is used in this content.
     * In audio contents, this can be safely cast to AudioSourceController*
     * and in video contents, to VideoSourceController*.
     */
    BaseSourceController *sourceController() const;

    /** \returns the controllers for the output sinks. Typically, there
     * is one sink controller for each participant in this content.
     * In audio contents, every sink controller can be safely cast to
     * AudioSinkController* and in video contents, to VideoSinkController*.
     */
    QSet<BaseSinkController*> sinkControllers() const;

Q_SIGNALS:
    void sinkControllerAdded(BaseSinkController *controller);
    void sinkControllerRemoved(BaseSinkController *controller);

private:
    explicit CallContentHandler(QObject *parent = 0);
    virtual ~CallContentHandler();

    friend class CallContentHandlerPrivate; //emits the signals
    friend class PendingCallContentHandler; //calls the constructor and uses d
    friend class CallChannelHandlerPrivate; //calls the destructor
    CallContentHandlerPrivate *const d;
};

#endif // CALLCONTENTHANDLER_H
