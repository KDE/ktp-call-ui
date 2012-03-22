/*
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>
    Copyright (C) 2010-2011 Collabora Ltd. <info@collabora.co.uk>
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
#include "call-window.h"
#include "ui_call-window.h"
#include "status-area.h"
#include "dtmf-handler.h"
#include "../libktpcall/call-channel-handler.h"
#include <QtGui/QCloseEvent>
#include <QtGui/QDockWidget>
#include <KDebug>
#include <KLocalizedString>
#include <KToggleAction>
#include <KActionCollection>

struct CallWindow::Private
{
    Private() : callEnded(false) {}

    Tp::CallChannelPtr callChannel;
    CallChannelHandler *channelHandler;
    StatusArea *statusArea;
    KToggleAction *muteAction;
    Ui::CallWindow ui;
    bool callEnded;
};

/*! This constructor is used to handle an incoming call, in which case
 * the specified \a channel must be ready and the call must have been accepted.
 */
CallWindow::CallWindow(const Tp::CallChannelPtr & callChannel)
    : KXmlGuiWindow(), d(new Private)
{
    d->callChannel = callChannel;
    connect(callChannel.data(), SIGNAL(callStateChanged(Tp::CallState)),
            this, SLOT(setState(Tp::CallState)));

    //create the channel handler
    d->channelHandler = new CallChannelHandler(callChannel, this);
    connect(d->channelHandler, SIGNAL(callEnded()), this, SLOT(onCallEnded()));

    //create ui
    d->ui.setupUi(this);
    d->statusArea = new StatusArea(statusBar());
    setupActions(); //must be called after creating the channel handler
    setupGUI(QSize(430, 300), ToolBar | Keys | StatusBar | Create, QLatin1String("callwindowui.rc"));
    setAutoSaveSettings(QLatin1String("CallWindow"), false);

    //enable dtmf
    if (callChannel->interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_DTMF)) {
        kDebug() << "Creating DTMF handler";
        DtmfHandler *handler = new DtmfHandler(callChannel, this);
        handler->connectDtmfWidget(d->ui.dtmfWidget);
    } else {
        d->ui.dtmfWidget->setEnabled(false);
    }
    d->ui.dtmfDock->hide();

    setState(callChannel->callState());
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
    connect(hangupAction, SIGNAL(triggered()), this, SLOT(hangup()));
    actionCollection()->addAction("hangup", hangupAction);
}

void CallWindow::setState(Tp::CallState state)
{
    //TODO take into account the CallFlags
    switch (state) {
    case Tp::CallStatePendingInitiator:
    case Tp::CallStateInitialising:
    case Tp::CallStateInitialised:
    case Tp::CallStateAccepted:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Connecting..."));
        break;
    case Tp::CallStateActive:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Talking..."));
        d->statusArea->startDurationTimer();
        break;
    case Tp::CallStateEnded:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Disconnected."));
        d->statusArea->stopDurationTimer();
        break;
    default:
        Q_ASSERT(false);
    }
}

void CallWindow::onCallEnded()
{
    d->callEnded = true;
    QTimer::singleShot(5000, this, SLOT(close()));
}

void CallWindow::onAudioContentAdded(CallContentHandler *contentHandler)
{
    d->statusArea->showAudioStatusIcon(true);
    d->muteAction->setEnabled(true);
    d->muteAction->setChecked(contentHandler->sourceController()->sourceEnabled());
    connect(d->muteAction, SIGNAL(toggled(bool)),
            contentHandler->sourceController(), SLOT(setSourceEnabled(bool)));
}

void CallWindow::onAudioContentRemoved(CallContentHandler *contentHandler)
{
    Q_UNUSED(contentHandler);
    d->statusArea->showAudioStatusIcon(false);
    d->muteAction->setEnabled(false);
}

void CallWindow::onVideoContentAdded(CallContentHandler *contentHandler)
{
    Q_UNUSED(contentHandler);
    d->statusArea->showVideoStatusIcon(true);
}

void CallWindow::onVideoContentRemoved(CallContentHandler *contentHandler)
{
    Q_UNUSED(contentHandler);
    d->statusArea->showVideoStatusIcon(false);
}

void CallWindow::hangup()
{
    kDebug();
    d->callChannel->hangup();
}

void CallWindow::closeEvent(QCloseEvent *event)
{
    if (!d->callEnded) {
        kDebug() << "Ignoring close event";
        hangup();
        event->ignore();
    } else {
        KXmlGuiWindow::closeEvent(event);
    }
}
