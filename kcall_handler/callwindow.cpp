/*
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

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
#include "dtmfhandler.h"
#include "kcallhandlersettings.h"
#include "../libkcallprivate/groupmembersmodel.h"
#include "../libtelepathy-kde-call/callchannelhandler.h"
#include <QtCore/QMetaObject>
#include <QtGui/QCloseEvent>
#include <QtGui/QLabel>
#include <QtGui/QDockWidget>
#include <KDebug>
#include <KLocalizedString>
#include <KToggleAction>
#include <KActionCollection>
#include <KStatusBar>
#include <TelepathyQt4/StreamedMediaChannel>
#include <TelepathyQt4/Connection>

enum TabIndices { VolumeTabIndex, VideoControlsTabIndex, DialpadTabIndex };

struct CallWindow::Private
{
    Tp::StreamedMediaChannelPtr channel;
    CallChannelHandler *channelHandler;
    KAction *hangupAction;
    KAction *sendVideoAction;
    bool sendingVideo;
    QTime callDuration;
    QTimer callDurationTimer;
    QDockWidget *videoDock;
    QLabel *statusLabel;
    Ui::CallWindow ui;
    CallLog *callLog;
    QPointer<QWidget> audioStatusIcon;
    QPointer<QWidget> videoStatusIcon;
    bool callEnded;
};

/*! This constructor is used to handle an incoming call, in which case
 * the specified \a channel must be ready and the call must have been accepted.
 */
CallWindow::CallWindow(const Tp::StreamedMediaChannelPtr & channel)
    : KXmlGuiWindow(), d(new Private)
{
    //setup the channel
    d->channel = channel;
    d->channel->acceptCall();
    d->callEnded = false;

    //create the gstreamer handler
    d->channelHandler = new CallChannelHandler(d->channel, this);
    connect(d->channelHandler, SIGNAL(participantJoined(CallParticipant*)),
            this, SLOT(onParticipantJoined(CallParticipant*)));
    connect(d->channelHandler, SIGNAL(participantLeft(CallParticipant*)),
            this, SLOT(onParticipantLeft(CallParticipant*)));
    connect(d->channelHandler, SIGNAL(error(QString)), this, SLOT(logErrorMessage(QString)));
    connect(d->channelHandler, SIGNAL(callEnded(QString)), this, SLOT(onCallEnded(QString)));

    //create ui
    setupUi(); //must be called after creating the state handler

    d->callLog = new CallLog(d->ui.logView, this);
    connect(d->callLog, SIGNAL(notifyUser()), d->ui.logDock, SLOT(show()));

    connect(&d->callDurationTimer, SIGNAL(timeout()), SLOT(onCallDurationTimerTimeout()));

    //enable dtmf
    if ( d->channel->interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_DTMF) ) {
        kDebug() << "Creating DTMF handler";
        DtmfHandler *handler = new DtmfHandler(d->channel, this);
        d->ui.tabWidget->setTabEnabled(DialpadTabIndex, true);
        d->ui.dtmfWidget->setEnabled(true);
        handler->connectDtmfWidget(d->ui.dtmfWidget);
    }

    //populate the participants dock
    GroupMembersModel *model = new GroupMembersModel(Tp::ChannelPtr::staticCast(d->channel), this);
    d->ui.participantsDock->setEnabled(true);
    d->ui.participantsListView->setModel(model);

    setState(Connecting);
}

CallWindow::~CallWindow()
{
    kDebug() << "Deleting CallWindow";
    delete d;
}

void CallWindow::setupActions()
{
    d->hangupAction = new KAction(KIcon("application-exit"), i18nc("@action", "Hangup"), this);
    connect(d->hangupAction, SIGNAL(triggered()), d->channelHandler, SLOT(hangup()));
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
    d->ui.participantsDock->setEnabled(false);
    //d->ui.tabWidget->setTabEnabled(VolumeTabIndex, false);
    d->ui.microphoneGroupBox->setEnabled(false);
    d->ui.speakersGroupBox->setEnabled(false);
    d->ui.tabWidget->setTabEnabled(VideoControlsTabIndex, false);
    d->ui.videoControlsTab->setEnabled(false);
    d->ui.tabWidget->setTabEnabled(DialpadTabIndex, false);
    d->ui.dtmfWidget->setEnabled(false);
}

void CallWindow::setState(State state)
{
    switch (state) {
    case Connecting:
        setStatus(i18nc("@info:status", "Connecting..."));
        break;
    case Connected:
        setStatus(i18nc("@info:status", "Talking..."));
        d->callDurationTimer.start(1000);
        break;
    case Disconnected:
        setStatus(i18nc("@info:status", "Disconnected."));
        disableUi();
        d->callDurationTimer.stop();
        d->callEnded = true;
        if ( KCallHandlerSettings::closeOnDisconnect() && !d->callLog->errorHasBeenLogged() ) {
            QTimer::singleShot(KCallHandlerSettings::closeOnDisconnectTimeout() * 1000,
                               this, SLOT(close()));
        }
        break;
    default:
        Q_ASSERT(false);
    }
}

void CallWindow::setStatus(const QString & msg)
{
    d->statusLabel->setText(msg);
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
    //d->stateHandler->setSendVideo(!d->sendingVideo);
}

void CallWindow::onParticipantJoined(CallParticipant *participant)
{
    connect(participant, SIGNAL(audioStreamAdded(CallParticipant*)),
            this, SLOT(onParticipantAudioStreamAdded(CallParticipant*)));
    connect(participant, SIGNAL(audioStreamRemoved(CallParticipant*)),
            this, SLOT(onParticipantAudioStreamRemoved(CallParticipant*)));
    connect(participant, SIGNAL(videoStreamAdded(CallParticipant*)),
            this, SLOT(onParticipantVideoStreamAdded(CallParticipant*)));
    connect(participant, SIGNAL(videoStreamRemoved(CallParticipant*)),
            this, SLOT(onParticipantVideoStreamRemoved(CallParticipant*)));

    if (participant->isMyself()) {
        setState(Connected);
    }
}

void CallWindow::onParticipantLeft(CallParticipant *participant)
{
    disconnect(participant, SIGNAL(audioStreamAdded(CallParticipant*)),
               this, SLOT(onParticipantAudioStreamAdded(CallParticipant*)));
    disconnect(participant, SIGNAL(audioStreamRemoved(CallParticipant*)),
               this, SLOT(onParticipantAudioStreamRemoved(CallParticipant*)));
    disconnect(participant, SIGNAL(videoStreamAdded(CallParticipant*)),
               this, SLOT(onParticipantVideoStreamAdded(CallParticipant*)));
    disconnect(participant, SIGNAL(videoStreamRemoved(CallParticipant*)),
               this, SLOT(onParticipantVideoStreamRemoved(CallParticipant*)));
}

void CallWindow::onParticipantAudioStreamAdded(CallParticipant *participant)
{
    if (participant->isMyself()) {
        onAudioStreamAdded();
        d->ui.microphoneGroupBox->setEnabled(true);
        d->ui.inputVolumeWidget->setVolumeControl(participant);
    } else {
        d->ui.speakersGroupBox->setEnabled(true);
        d->ui.outputVolumeWidget->setVolumeControl(participant);
    }
}

void CallWindow::onParticipantAudioStreamRemoved(CallParticipant *participant)
{
    if (participant->isMyself()) {
        onAudioStreamRemoved();
        d->ui.microphoneGroupBox->setEnabled(false);
        d->ui.inputVolumeWidget->setVolumeControl(NULL);
    } else {
        d->ui.speakersGroupBox->setEnabled(false);
        d->ui.outputVolumeWidget->setVolumeControl(NULL);
    }
}

void CallWindow::onParticipantVideoStreamAdded(CallParticipant *participant)
{
    QWidget *videoWidget = participant->videoWidget();
    setUpdatesEnabled(false); // reduce flickering. remember to enable it at the end again.

    if (participant->isMyself()) {
        onVideoStreamAdded();

        d->ui.tabWidget->setTabEnabled(VideoControlsTabIndex, true);
        d->ui.videoControlsTab->setEnabled(true);
        d->ui.videoInputBalanceWidget->setVideoBalanceControl(participant);

        videoWidget->setMinimumSize(160, 120);
        videoWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        d->ui.verticalLayout->insertWidget(0, videoWidget);

        // force update of the size hint
        d->ui.verticalLayout->update();
        d->ui.verticalLayout->activate();
    } else {
        videoWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        d->videoDock = new QDockWidget(this);
        d->videoDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        d->videoDock->setWidget(videoWidget);
        addDockWidget(Qt::LeftDockWidgetArea, d->videoDock);

        QCoreApplication::processEvents(); // force update of the size hint
    }

    resize(sizeHint());
    setUpdatesEnabled(true);
}

void CallWindow::onParticipantVideoStreamRemoved(CallParticipant *participant)
{
    setUpdatesEnabled(false); // reduce flickering. remember to enable it at the end again.

    if (participant->isMyself()) {
        onVideoStreamRemoved();

        d->ui.tabWidget->setTabEnabled(VideoControlsTabIndex, false);
        d->ui.videoControlsTab->setEnabled(false);
        d->ui.videoInputBalanceWidget->setVideoBalanceControl(NULL);

        QWidget *videoWidget = dynamic_cast<QWidget*>(d->ui.verticalLayout->takeAt(0));
        videoWidget->hide();
        videoWidget->deleteLater();

        // force update of the size hint
        d->ui.verticalLayout->update();
        d->ui.verticalLayout->activate();

        // no idea why resize needs to be done twice, but it works...
        // apparently the first time it only resizes horizontally and the second time it resizes vertically.
        resize(sizeHint());
        resize(sizeHint());
    } else {
        removeDockWidget(d->videoDock);
        d->videoDock->deleteLater();
        d->videoDock = NULL;

        QCoreApplication::processEvents(); // force update of the size hint
        resize(sizeHint());
    }

    setUpdatesEnabled(true);
}

void CallWindow::logErrorMessage(const QString & error)
{
    d->callLog->logMessage(CallLog::Error, error);
}

void CallWindow::onCallEnded(const QString & message)
{
    kDebug() << message;
    setState(Disconnected);
}

void CallWindow::closeEvent(QCloseEvent *event)
{
    if ( !d->callEnded ) {
        kDebug() << "Ignoring close event";
        d->channelHandler->hangup();
        event->ignore();
    } else {
        KXmlGuiWindow::closeEvent(event);
    }
}

#include "callwindow.moc"
