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
#include "volumedock.h"
#include "volumewidget.h"
#include "abstractmediahandler.h"
#include <QtCore/QMetaObject>
#include <QtGui/QCloseEvent>
#include <QtGui/QLabel>
#include <KDebug>
#include <KLocalizedString>
#include <KAction>
#include <KActionCollection>
#include <KStatusBar>
#include <TelepathyQt4/StreamedMediaChannel>

struct CallWindow::Private
{
    QLabel *dummyLabel;
    KAction *hangupAction;
    ChannelHandler *channelHandler;
    VolumeDock *volumeDock;
    QTime callDuration;
    QTimer callDurationTimer;
};

/*! This constructor is used to handle an incoming call, in which case
 * the specified \a channel must be ready and the call must have been accepted.
 */
CallWindow::CallWindow(Tp::StreamedMediaChannelPtr channel)
    : KXmlGuiWindow(), d(new Private)
{
    d->channelHandler = new ChannelHandler(channel, this);
    connect(d->channelHandler, SIGNAL(stateChanged(ChannelHandler::State)),
            SLOT(setState(ChannelHandler::State)));
    connect(d->channelHandler, SIGNAL(mediaHandlerCreated(AbstractMediaHandler*)),
            SLOT(onMediaHandlerCreated(AbstractMediaHandler*)));

    setupUi();

    connect(&d->callDurationTimer, SIGNAL(timeout()), SLOT(onCallDurationTimerTimeout()));
}

CallWindow::~CallWindow()
{
    kDebug() << "Deleting CallWindow";
    delete d;
}

void CallWindow::setupActions()
{
    d->hangupAction = new KAction(KIcon("application-exit"), i18nc("@action", "Hangup"), this);
    connect(d->hangupAction, SIGNAL(triggered()), d->channelHandler, SLOT(hangupCall()));
    actionCollection()->addAction("hangup", d->hangupAction);
}

void CallWindow::setupUi()
{
    d->dummyLabel = new QLabel("To be replaced by a real widget", this);
    setCentralWidget(d->dummyLabel);
    d->volumeDock = NULL;

    d->callDuration.setHMS(0, 0, 0);
    statusBar()->insertPermanentItem(d->callDuration.toString(), 1);

    setupActions();

    setAutoSaveSettings(QLatin1String("CallWindow"));
    setupGUI(QSize(320, 260), Default, QLatin1String("callwindowui.rc"));
}

void CallWindow::setState(ChannelHandler::State state)
{
    switch (state) {
    case ChannelHandler::Connecting:
        setStatus(i18nc("@info:status", "Connecting..."));
        d->hangupAction->setEnabled(false);
        break;
    case ChannelHandler::Ringing:
        setStatus(i18nc("@info:status", "Ringing..."));
        d->hangupAction->setEnabled(true);
        break;
    case ChannelHandler::InCall:
        setStatus(i18nc("@info:status", "Talking..."));
        d->hangupAction->setEnabled(true);
        d->callDurationTimer.start(1000);
        break;
    case ChannelHandler::HangingUp:
        setStatus(i18nc("@info:status", "Hanging up..."));
        d->hangupAction->setEnabled(false);
        break;
    case ChannelHandler::Disconnected:
        setStatus(i18nc("@info:status", "Disconnected."));
        d->hangupAction->setEnabled(false);
        d->callDurationTimer.stop();
        break;
    case ChannelHandler::Error:
        setStatus(i18nc("@info:status", "Disconnected with error."));
        d->hangupAction->setEnabled(false);
        d->callDurationTimer.stop();
        break;
    default:
        Q_ASSERT(false);
    }
}

void CallWindow::setStatus(const QString & msg)
{
    statusBar()->showMessage(msg);
}

void CallWindow::onCallEnded(bool hasError)
{
}

void CallWindow::onMediaHandlerCreated(AbstractMediaHandler *handler)
{
    d->volumeDock = new VolumeDock(this);
    d->volumeDock->inputVolumeWidget()->setAudioDevice(handler->audioInputDevice());
    d->volumeDock->outputVolumeWidget()->setAudioDevice(handler->audioOutputDevice());
    addDockWidget(Qt::BottomDockWidgetArea, d->volumeDock);
}

void CallWindow::onCallDurationTimerTimeout()
{
    d->callDuration = d->callDuration.addSecs(1);
    statusBar()->changeItem(d->callDuration.toString(), 1);
}

void CallWindow::closeEvent(QCloseEvent *event)
{
    if ( !d->channelHandler->requestClose() ) {
        kDebug() << "Ignoring close event";
        connect(d->channelHandler, SIGNAL(callEnded(bool)), SLOT(close()));
        event->ignore();
    }
    KXmlGuiWindow::closeEvent(event);
}

#include "callwindow.moc"
