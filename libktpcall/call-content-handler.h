/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
    Copyright (C) 2012 George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CALL_CONTENT_HANDLER_H
#define CALL_CONTENT_HANDLER_H

#include "volume-controller.h"
#include <TelepathyQt/CallContent>

class CallChannelHandler;

namespace KTpCallPrivate {
    class TfContentHandler;
    class TfAudioContentHandler;
    class TfVideoContentHandler;
}

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
    void localSendingStateChanged(bool sending);
    void remoteSendingStateChanged(const Tp::ContactPtr & contact, bool sending);

protected:
    friend class CallChannelHandler;
    CallContentHandler(KTpCallPrivate::TfContentHandler *handler, QObject *parent);
    virtual ~CallContentHandler();

    struct Private;
    Private *const d;
};


class AudioContentHandler : public CallContentHandler
{
    Q_OBJECT
public:
    VolumeController *inputVolumeControl() const;
    VolumeController *outputVolumeControl() const;

private:
    friend class CallChannelHandler;
    AudioContentHandler(KTpCallPrivate::TfAudioContentHandler *handler, QObject *parent);
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
    friend class CallChannelHandler;
    VideoContentHandler(KTpCallPrivate::TfVideoContentHandler *handler, QObject *parent);
    virtual ~VideoContentHandler() {}
};

#endif // CALL_CONTENT_HANDLER_H
