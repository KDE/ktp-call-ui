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

#include "status-area.h"
#include "dtmf-handler.h"
#include "dtmf-qml.h"
#include "../libktpcall/call-channel-handler.h"
#include "ktp_call_ui_debug.h"

#include <QCloseEvent>
#include <QVBoxLayout>
#include <QGraphicsObject>

#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Contact>

#include <KLocalizedString>
#include <KToggleAction>
#include <QAction>
#include <KActionCollection>
#include <KToolBar>
#include <KMessageWidget>
#include <KGuiItem>

#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QGst/ElementFactory>
#include <QGst/Ui/GraphicsVideoSurface>
#include <QGst/Init>

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
    bool callEnded;

    QVBoxLayout *layout;
    QmlInterface *qmlUi;
    QWidget *qmlUiContainer;
    KMessageWidget *errorWidget;
    DtmfQml *dtmfQml;

    KToggleAction *showMyVideoAction;
    KToggleAction *showDtmfAction;
    KToggleAction *sendVideoAction;
    KToggleAction *muteAction;
    QAction *holdAction;
    QAction *hangupAction;
    QAction *goToSystemTrayAction;
    QAction *restoreAction;
    KToggleAction *fullScreenAction;

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
    setupActions();

    //create ui
    setupQmlUi();
    d->statusArea = new StatusArea(statusBar());

    d->errorWidget->hide();
    d->errorWidget->setMessageType(KMessageWidget::Error);
    setupGUI(QSize(750, 450), ToolBar | Keys | StatusBar | Create, QLatin1String("callwindowui.rc"));
    setAutoSaveSettings(QLatin1String("CallWindow"), false);
    toolBar()->setToolButtonStyle(Qt::ToolButtonIconOnly);

    DtmfHandler *handler = new DtmfHandler(d->callChannel, this);
    d->dtmfQml = new DtmfQml(this);
    handler->connectDtmfQml(d->dtmfQml);

    //TODO handle member joining later
    if (!d->callChannel->remoteMembers().isEmpty()) {
        Tp::ContactPtr remoteMember = *d->callChannel->remoteMembers().begin();
        d->qmlUi->setLabel(i18n("Call with %1", remoteMember->alias()), remoteMember->avatarData().fileName );
        setWindowTitle(i18n("Call with %1", remoteMember->alias()));
    }
    setupSystemTray();
}

CallWindow::~CallWindow()
{
    qCDebug(KTP_CALL_UI) << "Deleting CallWindow";
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
            d->qmlUi->setHoldEnabled(true);
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
      d->qmlUi->setHoldEnabled(false);
    default:
        Q_ASSERT(false);
    }
}

void CallWindow::onContentAdded(CallContentHandler *contentHandler)
{
    qCDebug(KTP_CALL_UI) << "Content added:" << contentHandler->callContent()->name();

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
            qCCritical(KTP_CALL_UI) << "Multiple video contents are not supported";
            return;
        }

        d->videoContentHandler = qobject_cast<VideoContentHandler*>(contentHandler);
        Q_ASSERT(d->videoContentHandler);

        connect(d->videoContentHandler, SIGNAL(localSendingStateChanged(bool)),
                SLOT(onLocalVideoSendingStateChanged(bool)));
        connect(d->videoContentHandler, SIGNAL(remoteSendingStateChanged(Tp::ContactPtr,bool)),
                SLOT(onRemoteVideoSendingStateChanged(Tp::ContactPtr,bool)));

        d->statusArea->showVideoStatusIcon(true);
        d->showMyVideoAction->setEnabled(true);
        // would there be a good way of saving the users preference between runs?
        d->showMyVideoAction->setChecked(true);
    }
}

void CallWindow::onContentRemoved(CallContentHandler *contentHandler)
{
    qCDebug(KTP_CALL_UI) << "Content removed:" << contentHandler->callContent()->name();

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
    qCDebug(KTP_CALL_UI);

    if (sending) {
        changeVideoDisplayState(d->currentVideoDisplayState | LocalVideoPreview);
    } else {
        changeVideoDisplayState(d->currentVideoDisplayState & ~LocalVideoPreview);
    }
}

void CallWindow::onRemoteVideoSendingStateChanged(const Tp::ContactPtr & contact, bool sending)
{
    qCDebug(KTP_CALL_UI);

    if (d->remoteVideoContact && d->remoteVideoContact != contact) {
        qCCritical(KTP_CALL_UI) << "Multiple participants are not supported";
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

void CallWindow::changeVideoDisplayState(VideoDisplayFlags newState)
{
    VideoDisplayFlags oldState = d->currentVideoDisplayState;

    if (oldState.testFlag(LocalVideoPreview) && !newState.testFlag(LocalVideoPreview)) {
        d->videoContentHandler->unlinkVideoPreviewSink();
    } else if (!oldState.testFlag(LocalVideoPreview) && newState.testFlag(LocalVideoPreview)) {
        QGst::ElementPtr localVideoSink = d->qmlUi->getVideoPreviewSink();
        if (localVideoSink) {
            d->videoContentHandler->linkVideoPreviewSink(localVideoSink );
        }
    }

    if (oldState.testFlag(RemoteVideo) && !newState.testFlag(RemoteVideo)) {
        d->videoContentHandler->unlinkRemoteMemberVideoSink(d->remoteVideoContact);
    } else if (!oldState.testFlag(RemoteVideo) && newState.testFlag(RemoteVideo)) {
        QGst::ElementPtr remoteVideoSink = d->qmlUi->getVideoSink();
        if (remoteVideoSink) {
            d->videoContentHandler->linkRemoteMemberVideoSink(d->remoteVideoContact, remoteVideoSink);
        }
    }

    if (newState == NoVideo) {
        //d->ui.callStackedWidget->setCurrentIndex(0);
        d->qmlUi->setShowVideo(false);
    } else {
        //d->ui.callStackedWidget->setCurrentIndex(1);
        d->qmlUi->setShowVideo(true);
    }

    d->currentVideoDisplayState = newState;
}

void CallWindow::setupActions()
{
    d->showMyVideoAction = new KToggleAction(i18nc("@action", "Show my video"), this);
    d->showMyVideoAction->setIcon(QIcon::fromTheme("camera-web"));
    d->showMyVideoAction->setEnabled(false);
    d->showMyVideoAction->setChecked(false);
    actionCollection()->addAction("showMyVideo", d->showMyVideoAction);

    d->showDtmfAction = new KToggleAction(i18nc("@action", "Show dialpad"), this);
    d->showDtmfAction->setIcon(QIcon::fromTheme("phone"));
    d->showDtmfAction->setEnabled(false);
    connect(d->showDtmfAction, SIGNAL(toggled(bool)), SLOT(toggleDtmf(bool)));
    actionCollection()->addAction("showDtmf", d->showDtmfAction);

    d->goToSystemTrayAction = new QAction(i18nc("@action", "Hide window"), this);
    d->goToSystemTrayAction->setEnabled(true);
    connect(d->goToSystemTrayAction, SIGNAL(triggered(bool)), this, SLOT(hide()));
    actionCollection()->addAction("goToSystemTray", d->goToSystemTrayAction);

    d->restoreAction= new QAction(i18nc("@action", "Restore window"), this);
    d->restoreAction->setEnabled(true);
    connect(d->restoreAction, SIGNAL(triggered(bool)), this, SLOT(show()));

    //TODO implement this feature
    d->sendVideoAction = new KToggleAction(i18nc("@action", "Send video"), this);
    d->sendVideoAction->setIcon(QIcon::fromTheme("webcamsend"));
    d->sendVideoAction->setEnabled(false);
    actionCollection()->addAction("sendVideo", d->sendVideoAction);

    d->muteAction = new KToggleAction(QIcon::fromTheme("audio-volume-medium"), i18nc("@action", "Mute"), this);
    d->muteAction->setCheckedState(KGuiItem(i18nc("@action", "Mute"), QIcon::fromTheme("audio-volume-muted")));
    d->muteAction->setEnabled(false); //will be enabled later
    connect(d->muteAction, SIGNAL(toggled(bool)), SLOT(toggleMute(bool)));
    actionCollection()->addAction("mute", d->muteAction);

    d->holdAction = new QAction(i18nc("@action", "Hold"), this);
    d->holdAction->setIcon(QIcon::fromTheme("media-playback-pause"));
    d->holdAction->setEnabled(false); //will be enabled later
    connect(d->holdAction, SIGNAL(triggered()), SLOT(hold()));
    actionCollection()->addAction("hold", d->holdAction);

    d->hangupAction = new QAction(QIcon::fromTheme("call-stop"), i18nc("@action", "Hangup"), this);
    connect(d->hangupAction, SIGNAL(triggered()), SLOT(hangup()));
    actionCollection()->addAction("hangup", d->hangupAction);

    d->fullScreenAction = new KToggleAction(QIcon::fromTheme("view-fullscreen"),i18nc("@action", "Full Screen"), this);
    d->fullScreenAction->setEnabled(true);
    connect(d->fullScreenAction, SIGNAL(triggered()), SLOT(toggleFullScreen()));
    actionCollection()->addAction("fullScreen", d->fullScreenAction);
}

void CallWindow::checkEnableDtmf()
{
    bool dtmfSupported = false;
    Tp::Client::CallContentInterfaceDTMFInterface *dtmfInterface = 0;
    Q_FOREACH(const Tp::CallContentPtr & content, d->callChannel->contentsForType(Tp::MediaStreamTypeAudio)) {
        dtmfInterface = content->interface<Tp::Client::CallContentInterfaceDTMFInterface>();
        if (dtmfInterface) {
            qCDebug(KTP_CALL_UI) << "Does supportDTMF work? " << content->supportsDTMF() << dtmfInterface;
            dtmfSupported = true;
            break;
        }
    }

    qCDebug(KTP_CALL_UI) << "DTMF supported:" << dtmfSupported;
    d->showDtmfAction->setEnabled(dtmfSupported);

    if (!dtmfSupported) {
        toggleDtmf(false);
    }
}

void CallWindow::toggleDtmf(bool checked)
{
    if (checked) {
        exitFullScreen();
        d->dtmfQml->show();
    } else {
        d->dtmfQml->hide();
    }
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
    qCDebug(KTP_CALL_UI);
    d->callChannel->hangup();
}

void CallWindow::closeEvent(QCloseEvent *event)
{
    systemtrayicon->setActivateNext(false);
    if (!d->callEnded) {
        qCDebug(KTP_CALL_UI) << "Ignoring close event";
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
        qCDebug(KTP_CALL_UI) << "Call is currently being held, please wait before trying again!";
        return;
    }

    connect(holdRequest, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(holdOperationFinished(Tp::PendingOperation*)));
}

void CallWindow::holdOperationFinished(Tp::PendingOperation* operation)
{
    if (operation->isError()) {
        d->errorWidget->setText(i18nc("@info:error", "There was an error while pausing the call"));
        d->errorWidget->animatedShow();
        return;
    }
}

void CallWindow::onHoldStatusChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason)
{
    qCDebug(KTP_CALL_UI) << "Hold status changed" << state << " " << reason;

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
        d->qmlUi->setHoldEnabled(true);
        d->holdAction->setIcon(QIcon::fromTheme("media-playback-start"));
        d->qmlUi->setChangeHoldIcon("start");
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
        d->qmlUi->setHoldEnabled(true);
        d->holdAction->setIcon(QIcon::fromTheme("media-playback-pause"));
        d->qmlUi->setChangeHoldIcon("pause");
        d->statusArea->startDurationTimer();
        break;

    case Tp::LocalHoldStatePendingHold:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Putting the call on hold..."));
        d->holdAction->setEnabled(false);
        d->qmlUi->setHoldEnabled(false);
        break;

    case Tp::LocalHoldStatePendingUnhold:
        d->statusArea->setMessage(StatusArea::Status, i18nc("@info:status", "Resuming the call..."));
        d->holdAction->setEnabled(false);
        d->qmlUi->setHoldEnabled(false);
        break;

    default:
        d->statusArea->setMessage(StatusArea::Error, i18nc("@info:error", "Internal Error"));
    }
}

void CallWindow::setupSystemTray()
{
    QMenu *trayIconMenu = new QMenu(this);
    systemtrayicon = new SystemTrayIcon(this);

    //Save the title
    trayIconMenu->setTitle(windowTitle());

    //Show the title
    trayIconMenu->addAction(windowTitle());
    trayIconMenu->addSeparator();

    //Actions
    trayIconMenu->addAction(d->hangupAction);
    trayIconMenu->addAction(d->holdAction);
    trayIconMenu->addAction(d->sendVideoAction);
    trayIconMenu->addAction(d->muteAction);
    trayIconMenu->addAction(d->restoreAction);
    trayIconMenu->addAction(KStandardAction::close(this, SLOT(close()), actionCollection()));

    systemtrayicon->setAssociatedWidget(this);

    //Restore when left click
    connect(systemtrayicon, SIGNAL(activateRequested(bool,QPoint)), this, SLOT(show()));
    systemtrayicon->setToolTip("call-start", windowTitle(),"");

    systemtrayicon->setContextMenu(trayIconMenu);
}

void CallWindow::showEvent(QShowEvent* event)
{
    KXmlGuiWindow::showEvent(event);
    systemtrayicon->setStatus(KStatusNotifierItem::Passive);
}

void CallWindow::hideEvent(QHideEvent* event)
{
    if (isHidden()) {
        systemtrayicon->show();
    }
    KXmlGuiWindow::hideEvent(event);
}


/*! Creates a QmlInterface and sets it as central widget.
 * \a Ekaitz.
 */
void CallWindow::setupQmlUi()
{
    d->layout = new QVBoxLayout(this);
    QWidget *centralWidget = new QWidget(this);
    d->qmlUi = new QmlInterface( this );
    d->errorWidget = new KMessageWidget( this );

    d->qmlUiContainer = QWidget::createWindowContainer(d->qmlUi);

    d->layout->addWidget(d->errorWidget);
    d->layout->addWidget(d->qmlUiContainer);

    centralWidget->setLayout(d->layout);
    setCentralWidget(centralWidget);

    // grab root object so we can access QML signals.
    QObject *root = d->qmlUi->rootObject();

    //QML-UI <---> Actions
    //Show dialpad
    connect(root, SIGNAL(showDialpadClicked(bool)), SLOT(toggleDtmf(bool)));
    connect(d->showDtmfAction, SIGNAL(toggled(bool)), root, SIGNAL(showDialpadChangeState(bool)));
    connect(root, SIGNAL(showDialpadClicked(bool)), d->showDtmfAction, SLOT(setChecked(bool)));
    //Mute <-> Sound activated
    connect(root,SIGNAL(muteClicked(bool)), SLOT(toggleMute(bool)));
    connect(d->muteAction, SIGNAL(toggled(bool)), root, SIGNAL(muteClicked(bool)));
    connect(root,SIGNAL(muteClicked(bool)), d->muteAction, SLOT(setChecked(bool)));
    //Hold
    connect(root,SIGNAL(holdClicked()),SLOT(hold()));
    //Hangup
    connect(root,SIGNAL(hangupClicked()),SLOT(hangup()));
    //Exit FullScreen
    connect(root,SIGNAL(exitFullScreen()),SLOT(exitFullScreen()));
}

/*!This function makes the central QML widget go to full screen. To exit fullScreen mode, press \a Esc.
 * \a Ekaitz.
 * \sa QmlInterface::exitFullScreen(), hideWithSystemTray(), showWithSystemTray().
 */
void CallWindow::toggleFullScreen()
{
    if (d->qmlUiContainer->isFullScreen()) {
        d->fullScreenAction->setChecked(false);
        d->qmlUiContainer->showNormal();
        d->layout->addWidget(d->qmlUiContainer);
        show();
    } else {
        d->fullScreenAction->setChecked(true);
        d->layout->removeWidget(d->qmlUiContainer);
        d->qmlUiContainer->setParent(0);
        d->qmlUiContainer->showFullScreen();
        hide();
    }
}

void CallWindow::exitFullScreen()
{
    if (d->qmlUiContainer->isFullScreen()) {
        // avoid duplicating isFullScreen true branch.
        toggleFullScreen();
    }
}
