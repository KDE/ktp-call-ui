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
#ifndef SINK_MANAGER_H
#define SINK_MANAGER_H

#include "tf-content-handler.h"
#include "sink-controllers.h"
#include <TelepathyQt/Types>

namespace KTpCallPrivate {

class SinkManager : public QObject
{
    Q_OBJECT
public:
    explicit SinkManager(TfContentHandler *parent);
    virtual ~SinkManager();

    TfContentHandler *contentHandler() const { return static_cast<TfContentHandler*>(parent()); }

    /* Called before the destructor to release all remaining sink controllers */
    void cleanup();

    /* Called from the streaming thread */
    void handleNewSinkPad(uint contactHandle, const QGst::PadPtr & pad);

private Q_SLOTS:
    void handleNewSinkPadAsync(uint contactHandle);
    void onPendingContactsFinished(Tp::PendingOperation*);
    void destroyController(KTpCallPrivate::BaseSinkController *controller);

private:
    /* May be called from the streaming thread */
    void onPadUnlinked(const QGst::PadPtr & srcPad);

Q_SIGNALS:
    void controllerCreated(KTpCallPrivate::BaseSinkController *controller);
    void controllerDestroyed(KTpCallPrivate::BaseSinkController *controller);

private:
    Tp::ContactManagerPtr m_contactManager;

    QMutex m_mutex;
    QHash<uint, BaseSinkController*> m_controllersWaitingForContact;
    QHash<QGst::PadPtr, BaseSinkController*> m_controllers;
    QSet<BaseSinkController*> m_releasedControllers;
};

} // KTpCallPrivate

#endif
