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

#ifndef TF_CHANNEL_HANDLER_H
#define TF_CHANNEL_HANDLER_H

#include "tf-content-handler-factory.h"

#include <QList>
#include <QHash>
#include <QGst/Pipeline>

namespace KTpCallPrivate {

class TfChannelHandler : public QObject
{
    Q_OBJECT

public:
    explicit TfChannelHandler(const Tp::CallChannelPtr & channel,
            TfContentHandlerFactory::Constructor factoryCtor,
            QObject *parent = 0);
    virtual ~TfChannelHandler();

    Tp::CallChannelPtr callChannel() const { return m_callChannel; }
    QTf::ChannelPtr tfChannel() const { return m_tfChannel; }
    QGst::PipelinePtr pipeline() const { return m_pipeline; }

    void shutdown();

Q_SIGNALS:
    void channelClosed();
    void contentAdded(KTpCallPrivate::TfContentHandler*);
    void contentRemoved(KTpCallPrivate::TfContentHandler*);

private Q_SLOTS:
    void init();
    void onPendingTfChannelFinished(Tp::PendingOperation *op);
    void onCallChannelInvalidated();

private:
    void onTfChannelClosed();
    void onContentAdded(const QTf::ContentPtr & tfContent);
    void onContentRemoved(const QTf::ContentPtr & tfContent);
    void onFsConferenceAdded(const QGst::ElementPtr & conference);
    void onFsConferenceRemoved(const QGst::ElementPtr & conference);
    void onBusMessage(const QGst::MessagePtr & message);

private:
    Tp::CallChannelPtr m_callChannel;
    QTf::ChannelPtr m_tfChannel;
    QGst::PipelinePtr m_pipeline;

    TfContentHandlerFactory *m_factory;

    uint m_channelClosedCounter;
    QList<QGlib::ObjectPtr> m_fsElementAddedNotifiers;

    QHash<QTf::ContentPtr, TfContentHandler*> m_contents;
};

} // KTpCallPrivate

#endif
