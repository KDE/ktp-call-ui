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
#include "ui_callwindow.h"
#include "abstractmediahandler.h"
#include "dtmfhandler.h"
#include "kcallhandlersettings.h"
#include "../libkcallprivate/groupmembersmodel.h"
#include <QtCore/QMetaObject>
#include <QtGui/QCloseEvent>
#include <QtGui/QLabel>
#include <KDebug>
#include <KLocalizedString>
#include <KToggleAction>
#include <KActionCollection>
#include <KStatusBar>
#include <TelepathyQt4/StreamedMediaChannel>

enum TabIndices { VolumeTabIndex, VideoControlsTabIndex, DialpadTabIndex };

struct CallWindow::Private
{
    KAction *hangupAction;
    KAction *sendVideoAction;
    bool sendingVideo;
    ChannelHandler *channelHandler;
    QTime callDuration;
    QTimer callDurationTimer;
    QHash<uint, QDockWidget*> videoOutputWidgets;
    QLabel *statusLabel;
    Ui::CallWindow ui;
    CallLog *callLog;
    QPointer<QWidget> audioStatusIcon;
    QPointer<QWidget> videoStatusIcon;
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
    connect(d->channelHandler, SIGNAL(dtmfHandlerCreated(DtmfHandler*)),
            SLOT(onDtmfHandlerCreated(DtmfHandler*)));
    connect(d->channelHandler, SIGNAL(audioStreamAdded()), SLOT(onAudioStreamAdded()));
    connect(d->channelHandler, SIGNAL(audioStreamRemoved()), SLOT(onAudioStreamRemoved()));
    connect(d->channelHandler, SIGNAL(videoStreamAdded()), SLOT(onVideoStreamAdded()));
    connect(d->channelHandler, SIGNAL(videoStreamRemoved()), SLOT(onVideoStreamRemoved()));
    connect(d->channelHandler, SIGNAL(sendVideoStateChanged(bool)),
            SLOT(onSendVideoStateChanged(bool)));

    setupUi();

    d->callLog = new CallLog(d->ui.logView, this);
    connect(d->channelHandler, SIGNAL(logMessage(CallLog::LogType, QString)),
            d->callLog, SLOT(logMessage(CallLog::LogType, QString)));
    connect(d->callLog, SIGNAL(notifyUser()), d->ui.logDock, SLOT(show()));

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

    d->sendVideoAction = new KAction(this);
    d->sendVideoAction->setIcon(KIcon("webcamsend"));
    connect(d->sendVideoAction, SIGNAL(triggered()), this, SLOT(toggleSendVideo()));
    actionCollection()->addAction("sendVideo", d->sendVideoAction);
    d->sendingVideo = true;
    onSendVideoStateChanged(false); //to initialize the text of the action

    QAction *showParticipants = d->ui.participantsDock->toggleViewAction();
    showParticipants->setText(i18nc("@action:inmenu", "Show participants"));
    showParticipants->setIcon(KIcon("system-users"));
    actionCollection()->addAction("showParticipants", showParticipants);

    QAction *showLog = d->ui.logDock->toggleViewAction();
    showLog->setText(i18nc("@action:inmenu", "Show log"));
    actionCollection()->addAction("showLog", showLog);
}

void CallWindow::setupUi()
{
    d->ui.setupUi(this);
    setupActions();

    d->ui.participantsDock->hide();
    d->ui.logDock->hide();
    disableUi();

    d->callDuration.setHMS(0, 0, 0);
    statusBar()->insertPermanentItem(d->callDuration.toString(), 1);

    d->statusLabel = new QLabel;
    statusBar()->addWidget(d->statusLabel);

    setupGUI(QSize(330, 330), ToolBar | Keys | StatusBar | Create, QLatin1String("callwindowui.rc"));
    setAutoSaveSettings(QLatin1String("CallWindow"), false);
}

void CallWindow::disableUi()
{
    d->hangupAction->setEnabled(false);
    d->ui.participantsDock->setEnabled(false);
    //d->ui.tabWidget->setTabEnabled(VolumeTabIndex, false);
    d->ui.microphoneGroupBox->setEnabled(false);
    d->ui.speakersGroupBox->setEnabled(false);
    d->ui.tabWidget->setTabEnabled(VideoControlsTabIndex, false);
    d->ui.videoControlsTab->setEnabled(false);
    d->ui.tabWidget->setTabEnabled(DialpadTabIndex, false);
    d->ui.dtmfWidget->setEnabled(false);
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
        if ( KCallHandlerSettings::closeOnDisconnect() && !d->callLog->errorHasBeenLogged() ) {
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
    d->statusLabel->setText(msg);
}

void CallWindow::onMediaHandlerCreated(AbstractMediaHandler *handler)
{
    connect(handler, SIGNAL(audioInputDeviceCreated(QObject*)),
            SLOT(onAudioInputDeviceCreated(QObject*)));
    connect(handler, SIGNAL(audioInputDeviceDestroyed()), SLOT(onAudioInputDeviceDestroyed()));

    connect(handler, SIGNAL(audioOutputDeviceCreated(QObject*)),
            SLOT(onAudioOutputDeviceCreated(QObject*)));
    connect(handler, SIGNAL(audioOutputDeviceDestroyed()), SLOT(onAudioOutputDeviceDestroyed()));

    connect(handler, SIGNAL(videoInputDeviceCreated(QObject*, QWidget*)),
            SLOT(onVideoInputDeviceCreated(QObject*, QWidget*)));
    connect(handler, SIGNAL(videoInputDeviceDestroyed()), SLOT(onVideoInputDeviceDestroyed()));

    connect(handler, SIGNAL(videoOutputWidgetCreated(QWidget*, uint)),
            SLOT(onVideoOutputWidgetCreated(QWidget*, uint)));
    connect(handler, SIGNAL(closeVideoOutputWidget(uint)), SLOT(onCloseVideoOutputWidget(uint)));
}

void CallWindow::onAudioInputDeviceCreated(QObject *control)
{
    d->ui.microphoneGroupBox->setEnabled(true);
    d->ui.inputVolumeWidget->setVolumeControl(control);
}

void CallWindow::onAudioInputDeviceDestroyed()
{
    d->ui.microphoneGroupBox->setEnabled(false);
}

void CallWindow::onAudioOutputDeviceCreated(QObject *control)
{
    d->ui.speakersGroupBox->setEnabled(true);
    d->ui.outputVolumeWidget->setVolumeControl(control);
}

void CallWindow::onAudioOutputDeviceDestroyed()
{
    d->ui.speakersGroupBox->setEnabled(false);
}

void CallWindow::onVideoInputDeviceCreated(QObject *balanceControl, QWidget *videoWidget)
{
    d->ui.tabWidget->setTabEnabled(VideoControlsTabIndex, true);
    d->ui.videoControlsTab->setEnabled(true);
    d->ui.videoInputBalanceWidget->setVideoBalanceControl(balanceControl);

    setUpdatesEnabled(false); // reduce flickering. remember to enable it at the end again.

    videoWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    d->ui.verticalLayout->insertWidget(0, videoWidget);

    // force update of the size hint
    d->ui.verticalLayout->update();
    d->ui.verticalLayout->activate();

    resize(sizeHint());
    setUpdatesEnabled(true);
}

void CallWindow::onVideoInputDeviceDestroyed()
{
    d->ui.tabWidget->setTabEnabled(VideoControlsTabIndex, false);
    d->ui.videoControlsTab->setEnabled(false);
    d->ui.videoInputBalanceWidget->setVideoBalanceControl(NULL);

    // force update of the size hint
    d->ui.verticalLayout->update();
    d->ui.verticalLayout->activate();

    // no idea why resize needs to be done twice, but it works...
    // apparently the first time it only resizes horizontally and the second time it resizes vertically.
    resize(sizeHint());
    resize(sizeHint());
}

void CallWindow::onVideoOutputWidgetCreated(QWidget *widget, uint id)
{
    setUpdatesEnabled(false); // reduce flickering. remember to enable it at the end again.

    widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    QDockWidget *dock = new QDockWidget(this);
    dock->setObjectName("video-output-" + QString::number(id));
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dock->setWidget(widget);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    d->videoOutputWidgets[id] = dock;

    QCoreApplication::processEvents(); // force update of the size hint
    resize(sizeHint());
    setUpdatesEnabled(true);
}

void CallWindow::onCloseVideoOutputWidget(uint id)
{
    QDockWidget *dock = d->videoOutputWidgets.take(id);
    if ( !dock ) {
        return;
    }

    setUpdatesEnabled(false); // reduce flickering. remember to enable it at the end again.

    removeDockWidget(dock);
    dock->deleteLater();

    QCoreApplication::processEvents(); // force update of the size hint
    resize(sizeHint());
    setUpdatesEnabled(true);
}

void CallWindow::onGroupMembersModelCreated(GroupMembersModel *model)
{
    d->ui.participantsDock->setEnabled(true);
    d->ui.participantsListView->setModel(model);
}

void CallWindow::onDtmfHandlerCreated(DtmfHandler *handler)
{
    d->ui.tabWidget->setTabEnabled(DialpadTabIndex, true);
    d->ui.dtmfWidget->setEnabled(true);
    handler->connectDtmfWidget(d->ui.dtmfWidget);
}

void CallWindow::onCallDurationTimerTimeout()
{
    d->callDuration = d->callDuration.addSecs(1);
    statusBar()->changeItem(d->callDuration.toString(), 1);
}

void CallWindow::onAudioStreamAdded()
{
    if ( !d->audioStatusIcon ) {
        QLabel *label = new QLabel;
        label->setPixmap(KIcon("voicecall").pixmap(16));
        d->audioStatusIcon = label;
        statusBar()->insertPermanentWidget(1, d->audioStatusIcon);
    } else {
        statusBar()->insertPermanentWidget(1, d->audioStatusIcon);
        d->audioStatusIcon->show();
    }
}

void CallWindow::onAudioStreamRemoved()
{
    Q_ASSERT(d->audioStatusIcon);
    statusBar()->removeWidget(d->audioStatusIcon);
}

void CallWindow::onVideoStreamAdded()
{
    if ( !d->videoStatusIcon ) {
        QLabel *label = new QLabel;
        label->setPixmap(KIcon("camera-web").pixmap(16));
        d->videoStatusIcon = label;
        statusBar()->insertPermanentWidget(1, d->videoStatusIcon);
    } else {
        statusBar()->insertPermanentWidget(1, d->videoStatusIcon);
        d->videoStatusIcon->show();
    }
}

void CallWindow::onVideoStreamRemoved()
{
    Q_ASSERT(d->videoStatusIcon);
    statusBar()->removeWidget(d->videoStatusIcon);
}

void CallWindow::onSendVideoStateChanged(bool enabled)
{
    if ( enabled == d->sendingVideo ) {
        return;
    }

    if ( enabled ) {
        d->sendVideoAction->setText(i18nc("@action", "Stop sending video"));
    } else {
        d->sendVideoAction->setText(i18nc("@action", "Send video"));
    }

    d->sendingVideo = enabled;
}

void CallWindow::toggleSendVideo()
{
    d->channelHandler->setSendVideo(!d->sendingVideo);
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
