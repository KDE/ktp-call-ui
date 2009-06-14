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
#ifndef CALLWINDOWPART_H
#define CALLWINDOWPART_H

#include <KParts/Part>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/StreamedMediaChannel>
class CallWindowPartPrivate;

class CallWindowPart : public KParts::Part
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(CallWindowPart)
    Q_CLASSINFO("Interface version", "0.1")
public:
    CallWindowPart(QWidget *parentWidget, QObject *parent, const QVariantList &args);

public Q_SLOTS:
    /** Handles an existing Tp::StreamedMediaChannel. This can be used
     * to handle an incoming call, in which case it is assumed that the call
     * has already been accepted. It can also be used for outgoing calls.
     * The channel does not need to be ready.
     */
    void handleStreamedMediaChannel(Tp::StreamedMediaChannelPtr channel);

    /** Request to close the call window. It returns true if the
     * window can be closed or false if it cannot (and *must not*)
     * be closed yet. This is to ensure that the call is terminated
     * properly. If this slot returns false, it means that a call
     * is in progress. In this case, it will attempt to hangup
     * the call and callEnded() will be emited when the call has
     * successfully terminated and the window can be safely closed.
     */
    bool requestClose();

Q_SIGNALS:
    /** This signal is emmited when a call is terminated. @a hasError
     * indicates whether the call terminated because of an error or
     * it was normally terminated. When this signal is emited, it is safe
     * to delete this object (close the window), which means that
     * requestClose() will return true.
     */
    void callEnded(bool hasError);

private:
    CallWindowPartPrivate *const d_ptr;
};

#endif
