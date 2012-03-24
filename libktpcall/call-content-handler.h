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
#ifndef CALL_CONTENT_HANDLER_H
#define CALL_CONTENT_HANDLER_H

#include "volume-controller.h"
#include <TelepathyQt/CallContent>

class CallContentHandlerPrivate;
class PendingCallContentHandler;
class CallChannelHandlerPrivate;

/**
 * This class handles streaming in a telepathy Call channel Content.
 * Everything related to streaming is handled internally.
 * In audio contents, this class can be safely cast to AudioContentHandler
 * and on video contents, to VideoContentHandler. Those subclasses expose
 * all you need to interract with the internal gstreamer pipeline.
 */
class CallContentHandler : public QObject
{
    Q_OBJECT
public:
    /** \returns the telepathy content associated with this handler */
    Tp::CallContentPtr callContent() const;

    /**
     * \returns a set of contacts that are actively participating in
     * the call at the moment (i.e. there is media flowing from them to us)
     */
    Tp::Contacts remoteMembers() const;

Q_SIGNALS:
    void remoteMemberJoined(const Tp::ContactPtr & contact);
    void remoteMemberLeft(const Tp::ContactPtr & contact);

protected:
    explicit CallContentHandler(QObject *parent = 0);
    virtual ~CallContentHandler();

    friend class CallContentHandlerPrivate; //emits the signals
    friend class PendingCallContentHandler; //uses d
    friend class CallChannelHandlerPrivate; //calls the destructor
    CallContentHandlerPrivate *const d;
};


class AudioContentHandler : public CallContentHandler
{
    Q_OBJECT
public:
    VolumeController *sourceVolumeControl() const;
    VolumeController *remoteMemberVolumeControl(const Tp::ContactPtr & contact) const;

private:
    friend class PendingCallContentHandler;
    explicit AudioContentHandler(QObject *parent = 0);
    virtual ~AudioContentHandler() {}
};


class VideoContentHandler : public CallContentHandler
{
    Q_OBJECT
public:
    void linkVideoPreviewSink(const QGst::ElementPtr & sink);
    void unlinkVideoPreviewSink();
    void linkRemoteMemberVideoSink(const Tp::ContactPtr & contact, const QGst::ElementPtr & sink);
    void unlinkRemoteMemberVideoSink(const Tp::ContactPtr & contact);

private:
    friend class PendingCallContentHandler;
    explicit VideoContentHandler(QObject *parent = 0);
    virtual ~VideoContentHandler() {}
};

#endif // CALL_CONTENT_HANDLER_H
