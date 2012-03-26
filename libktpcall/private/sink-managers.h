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
#ifndef SINK_MANAGERS_H
#define SINK_MANAGERS_H

#include "sink-controllers.h"
#include <TelepathyQt/Types>
#include <TelepathyQt/CallContent>
#include <QGst/Pipeline>

class BaseSinkManager : public QObject
{
    Q_OBJECT
public:
    virtual ~BaseSinkManager();

    /* This needs to be called before the destructor.
     * It is a public function because it calls pure virtual functions internally,
     * so it cannot be called from inside the destructor.
     */
    void unlinkAllPads();

    /* Called from the streaming thread */
    void handleNewSinkPad(uint contactHandle, const QGst::PadPtr & pad);

    void setCallContent(const Tp::CallContentPtr & callContent);

protected:
    explicit BaseSinkManager(QObject *parent = 0);

    /* Called from the streaming thread to do the initial linking of srcPad to the sink */
    virtual BaseSinkController *createController(const QGst::PadPtr & srcPad) = 0;

    /* Called from the streaming thread when the src pad associated with ctrl is unlinked */
    virtual void releaseControllerData(BaseSinkController *ctrl) = 0;

private Q_SLOTS:
    void handleNewSinkPadAsync(uint contactHandle);
    void onStreamAdded(const Tp::CallStreamPtr & stream);
    void onRemoteSendingStateChanged(const QHash<Tp::ContactPtr, Tp::SendingState> & states);
    void destroyController(BaseSinkController *controller);

private:
    void checkForNewContacts(const QList<Tp::ContactPtr> & contacts);
    /* Called from the streaming thread */
    void onPadUnlinked(const QGst::PadPtr & srcPad);

Q_SIGNALS:
    void controllerCreated(BaseSinkController *controller);
    void controllerDestroyed(BaseSinkController *controller);

private:
    Tp::CallContentPtr m_content;
    QMutex m_mutex;
    QHash<uint, BaseSinkController*> m_controllersWaitingForContact;
    QHash<QGst::PadPtr, BaseSinkController*> m_controllers;
    QSet<BaseSinkController*> m_releasedControllers;
};


class AudioSinkManager : public BaseSinkManager
{
    Q_OBJECT
public:
    AudioSinkManager(const QGst::PipelinePtr & pipeline, QObject *parent = 0);
    virtual ~AudioSinkManager();

protected:
    virtual BaseSinkController *createController(const QGst::PadPtr & srcPad);
    virtual void releaseControllerData(BaseSinkController *priv);

private:
    void refSink();
    void unrefSink();

    QMutex m_mutex;
    QGst::PipelinePtr m_pipeline;
    QGst::ElementPtr m_sink;
    QGst::ElementPtr m_adder;
    int m_sinkRefCount;
};


class VideoSinkManager : public BaseSinkManager
{
    Q_OBJECT
public:
    explicit VideoSinkManager(const QGst::PipelinePtr & pipeline, QObject *parent = 0);

protected:
    virtual BaseSinkController *createController(const QGst::PadPtr & srcPad);
    virtual void releaseControllerData(BaseSinkController *priv);

private:
    QGst::PipelinePtr m_pipeline;
};

#endif //SINK_MANAGERS_H
