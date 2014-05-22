/*
    Copyright (C) 2009-2012 George Kiagiadakis <kiagiadakis.george@gmail.com>
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

#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Contact>

#include <KDebug>
#include <KLocalizedString>
#include <KToggleAction>
#include <KActionCollection>
#include <KToolBar>
#include <KMessageWidget>

#include <QGst/ElementFactory>

struct CallWindow::Private
{
    Private() :
        callEnded(false),
        currentVideoDisplayState(NoVideo),
        videoContentHandler(NULL)
    {}

    Tp::CallChannelPtr callChannel;
    CallChannelHandler *channelHandler;
    StatusArea *statusArea;
    Ui::CallWindow ui;
    bool callEnded;

    KToggleAction *showMyVideoAction;
    KToggleAction *showDtmfAction;
    KToggleAction *sendVideoAction;
    KToggleAction *muteAction;
    KAction *holdAction;
    KAction *hangupAction;

    VideoDisplayFlags currentVideoDisplayState;
    VideoContentHandler *videoContentHandler;
    Tp::ContactPtr remoteVideoContact;
};

/*! This constructor is used to handle an incoming call, in which case
 * the specified \a channel must be ready and the call must have been accepted.
 */
CallWindow::CallWindow(const Tp::CallChannelPtr & callChannel)
    : KXmlGuiWindow(), d(new Private)
{
    d->callChannel = callChannel;

    //create ui
    d->ui.setupUi(this);
    d->statusArea = new StatusArea(statusBar());
    d->ui.errorWidget->hide();
    d->ui.errorWidget->setMessageType(KMessageWidget::Error);
    setupActions();
    setupGUI(QSize(428, 395), ToolBar | Keys | StatusBar | Create, QLatin1String("callwindowui.rc"));
    setAutoSaveSettings(QLatin1String("CallWindow"), false);
    toolBar()->setToolButtonStyle(Qt::ToolButtonIconOnly);

    DtmfHandler *handler = new DtmfHandler(d->callChannel, this);
    handler->connectDtmfWidget(d->ui.dtmfWidget);

    //TODO handle member joining later
    if (!d->callChannel->remoteMembers().isEmpty()) {
        Tp::ContactPtr remoteMember = *d->callChannel->remoteMembers().begin();
        d->ui.remoteUserDisplayLabel->setText(QString::fromAscii(
                "<html><body>"
                "<p align=\"center\"><img src=\"%1\" /></p>"
                "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px;\">"
                    "<span style=\" font-weight:600;\">%2</span></p>"
                "</body></html>")
                .arg(remoteMember->avatarData().fileName, remoteMember->alias()));
        setWindowTitle(i18n("Call with %1", remoteMember->alias()));
    }
}

CallWindow::~CallWindow()
{
    kDebug() << "Deleting CallWindow";
    delete d;
}

void CallWindow::setStatus(Status status, const Tp::CallStateReason & reason)
{
    switch (status) {
    case StatusConnecting:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Connecting..."));
        break;
    case StatusRemoteRinging:
        d->statusArea->setMessage(StatusArea::Status,
                i18nc("@info:status the remote user's phone is ringing", "Ringing..."));
        break;
    case StatusRemoteAccepted:
        d->statusArea->setMessage(StatusArea::Status,
                i18nc("@info:status", "Remote user accepted the call, waiting for connection..."));
        break;
    case StatusActive:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Talking..."));
        d->statusArea->startDurationTimer();
        if (d->callChannel.data()->hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
            d->holdAction->setEnabled(true);
            connect(d->callChannel.data(), SIGNAL(localHoldStateChanged(Tp::LocalHoldState,Tp::LocalHoldStateReason)),
                    SLOT(onHoldStatusChanged(Tp::LocalHoldState,Tp::LocalHoldStateReason)));
        }
        break;
    case StatusDisconnected:
      {
        bool actorIsRemoteContact = false;
        Q_FOREACH(const Tp::ContactPtr & contact, d->callChannel->remoteMembers()) {
            if (contact->handle()[0] == reason.actor) {
                actorIsRemoteContact = true;
            }
        }

        QString message;
        switch(reason.reason) {
        case Tp::CallStateChangeReasonUserRequested:
            if (actorIsRemoteContact) {
                message = i18nc("@info:status", "Remote contact ended the call.");
            }
            //when disconnected normally, close the window after a while
            QTimer::singleShot(5000, this, SLOT(close()));
            break;
        case Tp::CallStateChangeReasonRejected:
            if (actorIsRemoteContact) {
                message = i18nc("@info:status", "Remote contact rejected the call.");
            }
            break;
        case Tp::CallStateChangeReasonNoAnswer:
            if (actorIsRemoteContact) {
                message = i18nc("@info:status", "Remote contact didn't answer.");
            }
            break;
        case Tp::CallStateChangeReasonInvalidContact:
            message = i18nc("@info:status", "Invalid number or contact.");
            break;
        case Tp::CallStateChangeReasonPermissionDenied:
            if (reason.DBusReason == TP_QT_ERROR_INSUFFICIENT_BALANCE) {
                message = i18nc("@info:status", "You don't have enough balance to make this call.");
            } else {
                message = i18nc("@info:status", "You don't have permission to make this call.");
            }
            break;
        case Tp::CallStateChangeReasonBusy:
            if (actorIsRemoteContact) {
                message = i18nc("@info:status", "Remote contact is currently busy.");
            }
            break;
        case Tp::CallStateChangeReasonInternalError:
            message = i18nc("@info:status", "Internal component error.");
            break;
        case Tp::CallStateChangeReasonServiceError:
            message = i18nc("@info:status", "Remote service error.");
            break;
        case Tp::CallStateChangeReasonNetworkError:
            message = i18nc("@info:status", "Network error.");
            break;
        case Tp::CallStateChangeReasonMediaError:
            if (reason.DBusReason == TP_QT_ERROR_MEDIA_CODECS_INCOMPATIBLE) {
                message = i18nc("@info:status", "Failed to negotiate codecs.");
            } else {
                message = i18nc("@info:status", "Unsupported media type.");
            }
            break;
        case Tp::CallStateChangeReasonConnectivityError:
            message = i18nc("@info:status", "Connectivity error.");
            break;
        case Tp::CallStateChangeReasonUnknown:
        default:
            message = i18nc("@info:status", "Unknown error.");
            break;
        }

        if (message.isEmpty()) {
            d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Disconnected."));
        } else {
            d->statusArea->setMessage(StatusArea::Status,
                    i18nc("@info:status the call was disconnected because...",
                          "Disconnected: %1", message));
        }

        changeVideoDisplayState(NoVideo);
        d->statusArea->stopDurationTimer();
        d->callEnded = true;
        break;
      }
      d->holdAction->setEnabled(false);
    default:
        Q_ASSERT(false);
    }
}

void CallWindow::onContentAdded(CallContentHandler *contentHandler)
{
    kDebug() << "Content added:" << contentHandler->callContent()->name();

    if (contentHandler->callContent()->type() == Tp::MediaStreamTypeAudio) {
        AudioContentHandler *audioContentHandler = qobject_cast<AudioContentHandler*>(contentHandler);
        Q_ASSERT(audioContentHandler);

        checkEnableDtmf();

        VolumeController *vol = audioContentHandler->inputVolumeControl();
        d->muteAction->setEnabled(vol->volumeControlSupported());
        connect(vol, SIGNAL(volumeControlSupportedChanged(bool)),
                d->muteAction, SLOT(setEnabled(bool)));
        d->muteAction->setChecked(vol->isMuted());
        connect(vol, SIGNAL(mutedChanged(bool)), d->muteAction, SLOT(setChecked(bool)));
        d->muteAction->setProperty("volumeController", QVariant::fromValue<QObject*>(vol));

        d->statusArea->showAudioStatusIcon(true);
    } else {
        if (d->videoContentHandler) {
            kError() << "Multiple video contents are not supported";
            return;
        }

        d->videoContentHandler = qobject_cast<VideoContentHandler*>(contentHandler);
        Q_ASSERT(d->videoContentHandler);

        connect(d->videoContentHandler, SIGNAL(localSendingStateChanged(bool)),
                SLOT(onLocalVideoSendingStateChanged(bool)));
        connect(d->videoContentHandler, SIGNAL(remoteSendingStateChanged(Tp::ContactPtr,bool)),
                SLOT(onRemoteVideoSendingStateChanged(Tp::ContactPtr,bool)));

        d->statusArea->showVideoStatusIcon(true);
    }
}

void CallWindow::onContentRemoved(CallContentHandler *contentHandler)
{
    kDebug() << "Content removed:" << contentHandler->callContent()->name();

    if (contentHandler->callContent()->type() == Tp::MediaStreamTypeAudio) {
        AudioContentHandler *audioContentHandler = qobject_cast<AudioContentHandler*>(contentHandler);
        Q_ASSERT(audioContentHandler);

        VolumeController *vol = audioContentHandler->inputVolumeControl();
        disconnect(vol, NULL, d->muteAction, NULL);
        d->muteAction->setEnabled(false);
        d->muteAction->setProperty("volumeController", QVariant());

        d->statusArea->showAudioStatusIcon(false);
    } else {
        if (d->videoContentHandler && d->videoContentHandler == contentHandler) {
            changeVideoDisplayState(NoVideo);
            d->videoContentHandler = NULL;
            d->remoteVideoContact.reset();
        }

        d->statusArea->showVideoStatusIcon(false);
    }
}

void CallWindow::onLocalVideoSendingStateChanged(bool sending)
{
    kDebug();

    if (sending) {
        changeVideoDisplayState(d->currentVideoDisplayState | LocalVideoPreview);
    } else {
        changeVideoDisplayState(d->currentVideoDisplayState & ~LocalVideoPreview);
    }
}

void CallWindow::onRemoteVideoSendingStateChanged(const Tp::ContactPtr & contact, bool sending)
{
    kDebug();

    if (d->remoteVideoContact && d->remoteVideoContact != contact) {
        kError() << "Multiple participants are not supported";
        return;
    }

    if (!d->remoteVideoContact) {
        d->remoteVideoContact = contact;
    }

    if (sending) {
        changeVideoDisplayState(d->currentVideoDisplayState | RemoteVideo);
    } else {
        changeVideoDisplayState(d->currentVideoDisplayState & ~RemoteVideo);
    }
}

QGst::ElementPtr CallWindow::tryVideoSink(const char *videoSink)
{
    QGst::ElementPtr sink = QGst::ElementFactory::make(videoSink);
    if (!sink) {
        kDebug() << "Could not make video sink" << videoSink;
        return sink;
     }

    if (!sink->setState(QGst::StateReady)) {
        kDebug() << "Video sink" << videoSink << "does not want to become ready";
        return QGst::ElementPtr();
    }

    kDebug() << "Using video sink" << videoSink;
    sink->setState(QGst::StateNull);
    return sink;
}

QGst::ElementPtr CallWindow::constructVideoSink()
{
    QGst::ElementPtr sink = tryVideoSink("xvimagesink");
    if (!sink) {
        sink = tryVideoSink("ximagesink");
    }

    sink->setProperty("force-aspect-ratio", true);

    return sink;

}
void CallWindow::changeVideoDisplayState(VideoDisplayFlags newState)
{
    VideoDisplayFlags oldState = d->currentVideoDisplayState;

    if (oldState.testFlag(LocalVideoPreview) && !newState.testFlag(LocalVideoPreview)) {
        d->videoContentHandler->unlinkVideoPreviewSink();
        d->ui.videoPreviewWidget->setVideoSink(QGst::ElementPtr());
    } else if (!oldState.testFlag(LocalVideoPreview) && newState.testFlag(LocalVideoPreview)) {
        QGst::ElementPtr localVideoSink = constructVideoSink();
        if (localVideoSink) {
            d->ui.videoPreviewWidget->setVideoSink(localVideoSink);
            d->videoContentHandler->linkVideoPreviewSink(localVideoSink);
        }
    }

    if (oldState.testFlag(RemoteVideo) && !newState.testFlag(RemoteVideo)) {
        d->videoContentHandler->unlinkRemoteMemberVideoSink(d->remoteVideoContact);
        d->ui.videoWidget->setVideoSink(QGst::ElementPtr());
    } else if (!oldState.testFlag(RemoteVideo) && newState.testFlag(RemoteVideo)) {
        QGst::ElementPtr remoteVideoSink = constructVideoSink();
        if (remoteVideoSink) {
            d->ui.videoWidget->setVideoSink(remoteVideoSink);
            d->videoContentHandler->linkRemoteMemberVideoSink(d->remoteVideoContact, remoteVideoSink);
        }
    }

    if (newState == NoVideo) {
        d->ui.callStackedWidget->setCurrentIndex(0);
    } else {
        d->ui.callStackedWidget->setCurrentIndex(1);

        if (newState.testFlag(LocalVideoPreview)) {
            d->ui.videoPreviewWidget->show();
        } else {
            d->ui.videoPreviewWidget->hide();
        }
    }

    d->currentVideoDisplayState = newState;
}

void CallWindow::setupActions()
{
    d->showMyVideoAction = new KToggleAction(i18nc("@action", "Show my video"), this);
    d->showMyVideoAction->setIcon(KIcon("camera-web"));
    d->showMyVideoAction->setEnabled(true);
    connect(d->showMyVideoAction, SIGNAL(toggled(bool)), this,SLOT(toggleShowMyVideo(bool)));
    actionCollection()->addAction("showMyVideo", d->showMyVideoAction);


    d->showDtmfAction = new KToggleAction(i18nc("@action", "Show dialpad"), this);
    d->showDtmfAction->setIcon(KIcon("phone"));
    d->showDtmfAction->setEnabled(false);
    connect(d->showDtmfAction, SIGNAL(toggled(bool)), SLOT(toggleDtmf(bool)));
    actionCollection()->addAction("showDtmf", d->showDtmfAction);

    //TODO implement this feature
    d->sendVideoAction = new KToggleAction(i18nc("@action", "Send video"), this);
    d->sendVideoAction->setIcon(KIcon("webcamsend"));
    d->sendVideoAction->setEnabled(false);
    actionCollection()->addAction("sendVideo", d->sendVideoAction);

    d->muteAction = new KToggleAction(KIcon("audio-volume-medium"), i18nc("@action", "Mute"), this);
    d->muteAction->setCheckedState(KGuiItem(i18nc("@action", "Mute"), KIcon("audio-volume-muted")));
    d->muteAction->setEnabled(false); //will be enabled later
    connect(d->muteAction, SIGNAL(toggled(bool)), SLOT(toggleMute(bool)));
    actionCollection()->addAction("mute", d->muteAction);

    d->holdAction = new KAction(i18nc("@action", "Hold"), this);
    d->holdAction->setIcon(KIcon("media-playback-pause"));
    d->holdAction->setEnabled(false); //will be enabled later
    connect(d->holdAction, SIGNAL(triggered()), SLOT(hold()));
    actionCollection()->addAction("hold", d->holdAction);

    d->hangupAction = new KAction(KIcon("call-stop"), i18nc("@action", "Hangup"), this);
    connect(d->hangupAction, SIGNAL(triggered()), SLOT(hangup()));
    actionCollection()->addAction("hangup", d->hangupAction);
}

void CallWindow::checkEnableDtmf()
{
    bool dtmfSupported = false;
    Q_FOREACH(const Tp::CallContentPtr & content, d->callChannel->contentsForType(Tp::MediaStreamTypeAudio)) {
        if (content->supportsDTMF()) {
            dtmfSupported = true;
            break;
        }
    }

    kDebug() << "DTMF supported:" << dtmfSupported;
    d->showDtmfAction->setEnabled(dtmfSupported);

    if (!dtmfSupported) {
        toggleDtmf(false);
    }
}

void CallWindow::toggleDtmf(bool checked)
{
    d->ui.dtmfStackedWidget->setCurrentIndex(checked ? 1 : 0);
}

void CallWindow::toggleMute(bool checked)
{
    //this slot is here to avoid connecting the mute button directly to the VolumeController
    //as there is a signal loop: toggled() -> setMuted() -> mutedChanged() -> setChecked()
    QObject *volControl = qvariant_cast<QObject*>(sender()->property("volumeController"));
    if (volControl) {
        sender()->blockSignals(true);
        volControl->setProperty("muted", checked);
        sender()->blockSignals(false);
    }
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

void CallWindow::hold()
{
    Tp::PendingOperation *holdRequest;
    if (d->callChannel.data()->localHoldState() == Tp::LocalHoldStateHeld) {
        holdRequest = d->callChannel->requestHold(false);
    } else if (d->callChannel.data()->localHoldState() == Tp::LocalHoldStateUnheld) {
        holdRequest = d->callChannel->requestHold(true);
    } else {
        kDebug() << "Call is currently being held, please wait before trying again!";
        return;
    }

    connect(holdRequest, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(holdOperationFinished(Tp::PendingOperation*)));
}

void CallWindow::holdOperationFinished(Tp::PendingOperation* operation)
{
    if (operation->isError()) {
        d->ui.errorWidget->setText(i18nc("@info:error", "There was an error while pausing the call"));
        d->ui.errorWidget->animatedShow();
        return;
    }
}

void CallWindow::onHoldStatusChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason)
{
    kDebug() << "Hold status changed" << state << " " << reason;

    switch (state) {
    case Tp::LocalHoldStateHeld:
        if (reason == Tp::LocalHoldStateReasonRequested) {
            d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Call held"));
        } else if (reason == Tp::LocalHoldStateReasonResourceNotAvailable) {
            d->statusArea->setMessage(StatusArea::Error, i18nc("@info:error", "Failed to put the call off hold: Some devices are not available"));
        } else if (reason == Tp::LocalHoldStateReasonNone) {
            d->statusArea->setMessage(StatusArea::Error, i18nc("@info:error", "Unknown error"));
        }
        d->holdAction->setEnabled(true);
        d->holdAction->setIcon(KIcon("media-playback-start"));
        d->statusArea->stopDurationTimer();
        break;

    case Tp::LocalHoldStateUnheld:
        if (reason == Tp::LocalHoldStateReasonRequested) {
            d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Talking..."));
        } else if (reason == Tp::LocalHoldStateReasonNone ||
            reason == Tp::LocalHoldStateReasonResourceNotAvailable) {
            d->statusArea->setMessage(StatusArea::Error, i18nc("@info:error", "Unknown error"));
        }
        d->holdAction->setEnabled(true);
        d->holdAction->setIcon(KIcon("media-playback-pause"));
        d->statusArea->startDurationTimer();
        break;

    case Tp::LocalHoldStatePendingHold:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Putting the call on hold..."));
        d->holdAction->setEnabled(false);
        break;

    case Tp::LocalHoldStatePendingUnhold:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Resuming the call..."));
        d->holdAction->setEnabled(false);
        break;

    default:
        d->statusArea->setMessage(StatusArea::Error, i18nc("@info:error", "Internal Error"));
    }
}

void CallWindow::toggleShowMyVideo(bool checked)
{
    if (checked) {
        d->ui.videoPreviewWidget->hide();
        d->ui.videoWidget->repaint();
    } else {
        d->ui.videoPreviewWidget->show();
    }
}