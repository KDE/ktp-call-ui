/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "callwindow.h"
#include <QtGui/QStackedWidget>
#include <QtGui/QLabel>
#include <KDebug>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/StreamedMediaChannel>

struct CallWindow::Private
{
    QStackedWidget *videoStackedWidget;
    QLabel *videoPlaceHolder;
    Tp::StreamedMediaChannelPtr channel;
};

CallWindow::CallWindow()
    : KXmlGuiWindow(), d(new Private)
{
    d->videoStackedWidget = new QStackedWidget(this);
    d->videoPlaceHolder = new QLabel;
    d->videoStackedWidget->addWidget(d->videoPlaceHolder);

    setCentralWidget(d->videoStackedWidget);

    setAutoSaveSettings(QLatin1String("CallWindow"));
    setupGUI(QSize(320, 260), KXmlGuiWindow::Default, QLatin1String("callwindowui.rc"));
}

CallWindow::~CallWindow()
{
    delete d;
}

void CallWindow::callContact(Tp::ContactPtr contact)
{
    setWindowTitle(contact->alias());

    Tp::ConnectionPtr connection = contact->manager()->connection();
    kDebug() << "Connection is ready" << connection->isReady();

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"), QString("StreamedMedia"));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"), Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"), contact->handle()[0]);

    Tp::PendingChannel *pendChan = connection->ensureChannel(request);
    connect(pendChan, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(callRequestFinished(Tp::PendingOperation*)));
}

void CallWindow::callRequestFinished(Tp::PendingOperation *op)
{
    if ( op->isError() ) {
        kDebug() << "Failed to call contact" << op->errorName() << op->errorMessage();
        return;
    }

    Tp::PendingChannel *pendChan = qobject_cast<Tp::PendingChannel*>(op);
    Q_ASSERT(pendChan);

    if ( pendChan->yours() ) {
        Tp::StreamedMediaChannelPtr channel(qobject_cast<Tp::StreamedMediaChannel*>(pendChan->channel().data()));
        Q_ASSERT(!channel.isNull());
        d->channel = channel;

        kDebug() << "Channel is ready" << channel->isReady();
        kDebug() << "Awaiting answer" << channel->awaitingRemoteAnswer();
        kDebug() << "Handler required" << channel->handlerStreamingRequired();
    } else {
        kDebug() << "Channel is not ours";
    }
}

#include "callwindow.moc"
