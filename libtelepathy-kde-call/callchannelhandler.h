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
#ifndef CALLCHANNELHANDLER_H
#define CALLCHANNELHANDLER_H

#include "callcontenthandler.h"
#include <TelepathyQt4Yell/CallChannel>

class CallChannelHandlerPrivate;

/** This class handles streaming in a telepathy Call channel.
 * To begin streaming, construct an instance of this class and use the
 * CallContentHandler objects that this class creates to control streaming
 * for the individual contents of the call.
 */
class CallChannelHandler : public QObject
{
    Q_OBJECT
public:
    explicit CallChannelHandler(const Tpy::CallChannelPtr & channel, QObject *parent = 0);
    virtual ~CallChannelHandler();

    QList<CallContentHandler*> contents() const;

Q_SIGNALS:
    void contentAdded(CallContentHandler *content);
    void contentRemoved(CallContentHandler *content);
    void callEnded();

private:
    friend class CallChannelHandlerPrivate;
    CallChannelHandlerPrivate *const d;
};

#endif // CALLCHANNELHANDLER_H
