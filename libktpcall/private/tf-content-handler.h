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

#ifndef TF_CONTENT_HANDLER_H
#define TF_CONTENT_HANDLER_H

#include "tf-channel-handler.h"

namespace KTpCallPrivate {

class SinkManager;
class BaseSinkController;

class TfContentHandler : public QObject
{
    Q_OBJECT

public:
    TfContentHandler(const QTf::ContentPtr & tfContent, TfChannelHandler *parent);
    virtual ~TfContentHandler();

    Tp::CallContentPtr callContent() const { return m_callContent; }
    QTf::ContentPtr tfContent() const { return m_tfContent; }
    TfChannelHandler *channelHandler() const { return static_cast<TfChannelHandler*>(parent()); }

    Tp::Contacts remoteMembers() const;
    BaseSinkController *sinkController(const Tp::ContactPtr & contact) const;

    /* Called before the destructor to cleanup SinkManager
     * and any other pipeline parts that the subclass maintains */
    virtual void cleanup();

    /* Called from the streaming thread to do the initial linking of srcPad to the sink */
    virtual BaseSinkController *createSinkController(const QGst::PadPtr & srcPad) = 0;

    /* Called from the streaming thread when the src pad associated with ctrl is unlinked */
    virtual void releaseSinkControllerData(BaseSinkController *ctrl) = 0;

Q_SIGNALS:
    void callContentReady(KTpCallPrivate::TfContentHandler *self);
    void localSendingStateChanged(bool sending);
    void remoteSendingStateChanged(const Tp::ContactPtr & contact, bool sending);

protected:
    /* Reimplement to handle the start-sending and stop-sending TfContent signals */
    virtual bool startSending() = 0;
    virtual void stopSending() = 0;

private:
    void onSrcPadAdded(uint contactHandle,
                       const QGlib::ObjectPtr & fsStream,
                       const QGst::PadPtr & pad);
    bool onStartSending();
    void onStopSending();
    bool onStartReceiving(void *handles, uint handleCount);
    void onStopReceiving(void *handles, uint handleCount);

private Q_SLOTS:
    void onControllerCreated(KTpCallPrivate::BaseSinkController *controller);
    void onControllerDestroyed(KTpCallPrivate::BaseSinkController *controller);

    void findCallContent();
    void onContentAdded(const Tp::CallContentPtr & callContent);

private:
    Tp::CallContentPtr m_callContent;
    QTf::ContentPtr m_tfContent;

    SinkManager *m_sinkManager;
    QHash<Tp::ContactPtr, QPair<BaseSinkController*, bool> > m_sinkControllers; //bool -> receiving state
    QHash<uint, Tp::ContactPtr> m_handlesToContacts;

    bool m_sending;
};

} // KTpCallPrivate

#endif
