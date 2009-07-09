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
#include "../libkcallprivate/kcallhandlersettingsdialog.h"
#include "../libkgstdevices/volumecontrolinterface.h"
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
    ChannelHandler *channelHandler;
    QTime callDuration;
    QTimer callDurationTimer;

    Ui::CallWindow ui;
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

    CallLog *callLog = new CallLog(d->ui.logView, this);
    connect(d->channelHandler, SIGNAL(logMessage(CallLog::LogType, QString)),
            callLog, SLOT(logMessage(CallLog::LogType, QString)));
    connect(callLog, SIGNAL(notifyUser()), d->ui.logDock, SLOT(show()));

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

    QAction *showParticipants = d->ui.participantsDock->toggleViewAction();
    showParticipants->setText(i18nc("@action:inmenu", "Show participants"));
    showParticipants->setIcon(KIcon("system-users"));
    actionCollection()->addAction("showParticipants", showParticipants);

    QAction *showLog = d->ui.logDock->toggleViewAction();
    showLog->setText(i18nc("@action:inmenu", "Show log"));
    actionCollection()->addAction("showLog", showLog);

    KStandardAction::preferences(this, SLOT(showSettingsDialog()), actionCollection());
}

void CallWindow::setupUi()
{
    d->ui.setupUi(this);
    setupActions();

    d->ui.videoInputWidget->hide();
    d->ui.participantsDock->hide();
    d->ui.logDock->hide();
    disableUi();

    d->callDuration.setHMS(0, 0, 0);
    statusBar()->insertPermanentItem(d->callDuration.toString(), 1);

    setupGUI(QSize(320, 260), ToolBar | Keys | StatusBar | Create, QLatin1String("callwindowui.rc"));
    setAutoSaveSettings(QLatin1String("CallWindow"));
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
    connect(handler, SIGNAL(audioInputDeviceCreated(VolumeControlInterface*)),
            SLOT(onAudioInputDeviceCreated(VolumeControlInterface*)));
    connect(handler, SIGNAL(audioInputDeviceDestroyed()), SLOT(onAudioInputDeviceDestroyed()));

    connect(handler, SIGNAL(audioOutputDeviceCreated(VolumeControlInterface*)),
            SLOT(onAudioOutputDeviceCreated(VolumeControlInterface*)));
    connect(handler, SIGNAL(audioOutputDeviceDestroyed()), SLOT(onAudioOutputDeviceDestroyed()));
}

void CallWindow::onAudioInputDeviceCreated(VolumeControlInterface *control)
{
    d->ui.microphoneGroupBox->setEnabled(true);
    d->ui.inputVolumeWidget->setVolumeControl(control);
}

void CallWindow::onAudioInputDeviceDestroyed()
{
    d->ui.microphoneGroupBox->setEnabled(false);
}

void CallWindow::onAudioOutputDeviceCreated(VolumeControlInterface *control)
{
    d->ui.speakersGroupBox->setEnabled(true);
    d->ui.outputVolumeWidget->setVolumeControl(control);
}

void CallWindow::onAudioOutputDeviceDestroyed()
{
    d->ui.speakersGroupBox->setEnabled(false);
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
