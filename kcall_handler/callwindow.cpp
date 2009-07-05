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
#include "participantsdock.h"
#include "dtmfhandler.h"
#include "kcallhandlersettings.h"
#include "../libkcallprivate/dtmfwidget.h"
#include "../libkcallprivate/kcallhandlersettingsdialog.h"
#include "../libkgstdevices/volumecontrolinterface.h"
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
    KAction *hangupAction;
    ChannelHandler *channelHandler;
    VolumeDock *volumeDock;
    ParticipantsDock *participantsDock;
    QDockWidget *dialpadDock;
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
    connect(d->channelHandler, SIGNAL(groupMembersModelCreated(GroupMembersModel*)),
            SLOT(onGroupMembersModelCreated(GroupMembersModel*)));

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

    KStandardAction::preferences(this, SLOT(showSettingsDialog()), actionCollection());
}

void CallWindow::setupUi()
{
    QLabel *dummyLabel = new QLabel("To be replaced by a real widget", this);
    setCentralWidget(dummyLabel);

    d->volumeDock = NULL;
    d->dialpadDock = NULL;

    //the corners should be occupied by the left and right dock widget areas
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);

    d->callDuration.setHMS(0, 0, 0);
    statusBar()->insertPermanentItem(d->callDuration.toString(), 1);

    setupActions();

    setupGUI(QSize(320, 260), ToolBar | Keys | StatusBar | Create, QLatin1String("callwindowui.rc"));
    setAutoSaveSettings(QLatin1String("CallWindow"));
}

void CallWindow::disableUi()
{
    d->participantsDock->setEnabled(false);
    d->hangupAction->setEnabled(false);

    if ( d->volumeDock ) {
        d->volumeDock->setEnabled(false);
    }

    if ( d->dialpadDock ) {
        d->dialpadDock->setEnabled(false);
    }
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
        disableUi();
        d->callDurationTimer.stop();
        if ( KCallHandlerSettings::closeOnDisconnect() ) {
            QTimer::singleShot(KCallHandlerSettings::closeOnDisconnectTimeout() * 1000,
                               this, SLOT(close()));
        }
        break;
    case ChannelHandler::Error:
        setStatus(i18nc("@info:status", "Disconnected with error."));
        disableUi();
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

void CallWindow::onMediaHandlerCreated(AbstractMediaHandler *handler)
{
    d->volumeDock = new VolumeDock(this);
    d->volumeDock->inputVolumeWidget()->setVolumeControl(handler->inputVolumeControl());
    d->volumeDock->outputVolumeWidget()->setVolumeControl(handler->outputVolumeControl());
    addDockWidget(Qt::BottomDockWidgetArea, d->volumeDock);
    restoreDockWidget(d->volumeDock);
}

void CallWindow::onGroupMembersModelCreated(GroupMembersModel *model)
{
    d->participantsDock = new ParticipantsDock(model, this);
    addDockWidget(Qt::RightDockWidgetArea, d->participantsDock);
    restoreDockWidget(d->participantsDock);
}

void CallWindow::onDtmfHandlerCreated(DtmfHandler *handler)
{
    d->dialpadDock = new QDockWidget(i18n("Dialpad"), this);
    d->dialpadDock->setObjectName("DialpadDock");
    DtmfWidget *dtmfWidget = new DtmfWidget(d->dialpadDock);
    d->dialpadDock->setWidget(dtmfWidget);
    handler->connectDtmfWidget(dtmfWidget);
    addDockWidget(Qt::RightDockWidgetArea, d->dialpadDock);
    restoreDockWidget(d->dialpadDock);
}

void CallWindow::onCallDurationTimerTimeout()
{
    d->callDuration = d->callDuration.addSecs(1);
    statusBar()->changeItem(d->callDuration.toString(), 1);
}

void CallWindow::showSettingsDialog()
{
    if ( !KCallHandlerSettingsDialog::showDialog() ) {
        new KCallHandlerSettingsDialog(this, KCallHandlerSettings::self());
    }
}

void CallWindow::closeEvent(QCloseEvent *event)
{
    if ( !d->channelHandler->requestClose() ) {
        kDebug() << "Ignoring close event";
        disconnect(d->channelHandler, SIGNAL(stateChanged(ChannelHandler::State)), this, 0);
        connect(d->channelHandler, SIGNAL(stateChanged(ChannelHandler::State)), SLOT(close()));
        event->ignore();
    } else {
        KXmlGuiWindow::closeEvent(event);
    }
}

#include "callwindow.moc"
