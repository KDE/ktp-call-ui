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
#include "statusarea.h"
#include "dtmfhandler.h"
#include "../libtelepathy-kde-call/callchannelhandler.h"
#include <QtCore/QMetaObject>
#include <QtGui/QCloseEvent>
#include <QtGui/QDockWidget>
#include <KDebug>
#include <KLocalizedString>
#include <KToggleAction>
#include <KActionCollection>
#include <TelepathyQt4/StreamedMediaChannel>
#include <TelepathyQt4/Connection>

struct CallWindow::Private
{
    Private() : callEnded(false) {}

    CallChannelHandler *channelHandler;
    StatusArea *statusArea;
    KToggleAction *muteAction;
    Ui::CallWindow ui;
    bool callEnded;
};

/*! This constructor is used to handle an incoming call, in which case
 * the specified \a channel must be ready and the call must have been accepted.
 */
CallWindow::CallWindow(const Tp::StreamedMediaChannelPtr & channel)
    : KXmlGuiWindow(), d(new Private)
{
    //create the channel handler
    d->channelHandler = new CallChannelHandler(channel, this);
    connect(d->channelHandler, SIGNAL(participantJoined(CallParticipant*)),
            this, SLOT(onParticipantJoined(CallParticipant*)));
    connect(d->channelHandler, SIGNAL(participantLeft(CallParticipant*)),
            this, SLOT(onParticipantLeft(CallParticipant*)));
    connect(d->channelHandler, SIGNAL(error(QString)), this, SLOT(onHandlerError(QString)));
    connect(d->channelHandler, SIGNAL(callEnded(QString)), this, SLOT(onCallEnded(QString)));

    //create ui
    d->ui.setupUi(this);
    d->statusArea = new StatusArea(statusBar());
    setupActions(); //must be called after creating the channel handler
    setupGUI(QSize(430, 300), ToolBar | Keys | StatusBar | Create, QLatin1String("callwindowui.rc"));
    setAutoSaveSettings(QLatin1String("CallWindow"), false);

    //enable dtmf
    if (channel->interfaces().contains(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_DTMF)) {
        kDebug() << "Creating DTMF handler";
        DtmfHandler *handler = new DtmfHandler(channel, this);
        handler->connectDtmfWidget(d->ui.dtmfWidget);
    } else {
        d->ui.dtmfWidget->setEnabled(false);
    }
    d->ui.dtmfDock->hide();

    setState(Connecting);
}

CallWindow::~CallWindow()
{
    kDebug() << "Deleting CallWindow";
    delete d;
}

void CallWindow::setupActions()
{
    QAction *showMyVideoAction = new KToggleAction(i18nc("@action", "Show my video"), this);
    showMyVideoAction->setIcon(KIcon("camera-web"));
    actionCollection()->addAction("showMyVideo", showMyVideoAction);

    QAction *showDtmfAction = d->ui.dtmfDock->toggleViewAction();
    showDtmfAction->setText(i18nc("@action", "Show dialpad"));
    showDtmfAction->setIcon(KIcon("phone"));
    actionCollection()->addAction("showDtmf", showDtmfAction);

    QAction *sendVideoAction = new KToggleAction(i18nc("@action", "Send video"), this);
    sendVideoAction->setIcon(KIcon("webcamsend"));
    sendVideoAction->setEnabled(false); //TODO implement this feature
    actionCollection()->addAction("sendVideo", sendVideoAction);

    d->muteAction = new KToggleAction(KIcon("audio-volume-medium"), i18nc("@action", "Mute"), this);
    d->muteAction->setCheckedState(KGuiItem(i18nc("@action", "Mute"), KIcon("audio-volume-muted")));
    d->muteAction->setEnabled(false); //will be enabled later
    actionCollection()->addAction("mute", d->muteAction);

    QAction *holdAction = new KToggleAction(i18nc("@action", "Hold"), this);
    holdAction->setIcon(KIcon("media-playback-pause"));
    holdAction->setEnabled(false); //TODO implement this feature
    actionCollection()->addAction("hold", holdAction);

    QAction *hangupAction = new KAction(KIcon("application-exit"), i18nc("@action", "Hangup"), this);
    connect(hangupAction, SIGNAL(triggered()), d->channelHandler, SLOT(hangup()));
    actionCollection()->addAction("hangup", hangupAction);
}

void CallWindow::setState(State state)
{
    switch (state) {
    case Connecting:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Connecting..."));
        break;
    case Connected:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Talking..."));
        d->statusArea->startDurationTimer();
        break;
    case Disconnected:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Disconnected."));
        d->statusArea->stopDurationTimer();
        d->callEnded = true;
        QTimer::singleShot(5000, this, SLOT(close()));
        break;
    default:
        Q_ASSERT(false);
    }
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

    if (!participant->isMyself()) {
        setState(Connected);
    }
}

void CallWindow::onParticipantAudioStreamAdded(CallParticipant *participant)
{
    if (participant->isMyself()) {
        d->statusArea->showAudioStatusIcon(true);
        d->muteAction->setEnabled(true);
        d->muteAction->setChecked(participant->isMuted());
        connect(d->muteAction, SIGNAL(toggled(bool)), participant, SLOT(setMuted(bool)));
    }
}

void CallWindow::onParticipantAudioStreamRemoved(CallParticipant *participant)
{
    if (participant->isMyself()) {
        d->statusArea->showAudioStatusIcon(false);
        d->muteAction->setEnabled(false);
        d->muteAction->disconnect(participant);
    }
}

void CallWindow::onParticipantVideoStreamAdded(CallParticipant *participant)
{
    if (participant->isMyself()) {
        d->statusArea->showVideoStatusIcon(true);
    }
}

void CallWindow::onParticipantVideoStreamRemoved(CallParticipant *participant)
{
    if (participant->isMyself()) {
        d->statusArea->showVideoStatusIcon(false);
    }
}

void CallWindow::onHandlerError(const QString & error)
{
    d->statusArea->setMessage(StatusArea::Error, error);
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
