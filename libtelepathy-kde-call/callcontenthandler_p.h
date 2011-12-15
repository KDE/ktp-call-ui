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
#ifndef CALLCONTENTHANDLER_P_H
#define CALLCONTENTHANDLER_P_H

#include "callcontenthandler.h"
#include "../libqtf/qtf.h"
#include <QGst/Pipeline>
#include <QGst/Pad>

class BaseSinkManager;

/* This class constructs a CallContentHandler. This operation is asynchronous
 * because we need to construct a CallContentHandler as soon as we receive
 * the TfContent from tp-farstream, however, because tp-qt4 and tp-glib check
 * dbus separately, tp-qt4 may not be synchronized with tp-glib and thus the
 * Tp::CallContent may not be available when the TfContent comes. This class
 * basically waits for tp-qt4 to synchronize.
 */
class PendingCallContentHandler : public QObject
{
    Q_OBJECT
public:
    PendingCallContentHandler(const Tpy::CallChannelPtr & callChannel,
                              const QTf::ContentPtr & tfContent,
                              const QGst::PipelinePtr & pipeline,
                              QObject *parent);

private Q_SLOTS:
    void findCallContent();
    void onContentAdded(const Tpy::CallContentPtr & callContent);

Q_SIGNALS:
    void ready(const QTf::ContentPtr & tfContent, CallContentHandler *contentHandler);

private:
    CallContentHandler *m_contentHandler;
    Tpy::CallChannelPtr m_callChannel;
    QTf::ContentPtr m_tfContent;
};


class CallContentHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    CallContentHandlerPrivate(CallContentHandler *qq) : QObject(), q(qq) {}
    virtual ~CallContentHandlerPrivate();

    /* Called from PendingCallContentHandler right after constructing the CallContentHandler */
    void init(const QTf::ContentPtr & tfContent, const QGst::PipelinePtr & pipeline);

    /* Called from PendingCallContentHandler when the CallContent is ready */
    void setCallContent(const Tpy::CallContentPtr & callContent);

private:
    void onSrcPadAdded(uint contactHandle,
                       const QGlib::ObjectPtr & fsStream,
                       const QGst::PadPtr & pad);
    bool onStartSending();
    bool onStopSending();

private Q_SLOTS:
    void onControllerCreated(BaseSinkController *controller);
    void onControllerDestroyed(BaseSinkController *controller);

public: //accessed from the public class
    BaseSourceController *m_sourceController;
    QSet<BaseSinkController*> m_sinkControllers;
    Tpy::CallContentPtr m_callContent;

private:
    CallContentHandler * const q;
    BaseSinkManager *m_sinkManager;
    QTf::ContentPtr m_tfContent;
    QGst::PipelinePtr m_pipeline;
    QGst::PadPtr m_sourceControllerPad;
    QGst::ElementPtr m_queue;
};

#endif // CALLCONTENTHANDLER_P_H
