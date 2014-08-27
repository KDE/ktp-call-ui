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
#ifndef CALL_WINDOW_H
#define CALL_WINDOW_H

#include <TelepathyQt/CallChannel>
#include <KXmlGuiWindow>
#include <QGst/Global>

#include "systemtray-icon.h"
#include "qml-interface.h"

class CallContentHandler;

class CallWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    explicit CallWindow(const Tp::CallChannelPtr & channel);
    virtual ~CallWindow();

    enum Status {
        StatusConnecting,
        StatusRemoteRinging,
        StatusRemoteAccepted,
        StatusActive,
        StatusDisconnected
    };
    void setStatus(Status status, const Tp::CallStateReason & reason = Tp::CallStateReason());

public Q_SLOTS:
    void onContentAdded(CallContentHandler *contentHandler);
    void onContentRemoved(CallContentHandler *contentHandler);
    void onLocalVideoSendingStateChanged(bool sending);
    void onRemoteVideoSendingStateChanged(const Tp::ContactPtr & contact, bool sending);

private:
    enum VideoDisplayFlag {
        NoVideo = 0,
        LocalVideoPreview = 0x1,
        RemoteVideo = 0x2
    };
    Q_DECLARE_FLAGS(VideoDisplayFlags, VideoDisplayFlag);

    void changeVideoDisplayState(VideoDisplayFlags newState);

    void setupActions();
    void setupQmlUi();
    void checkEnableDtmf();
    QGst::ElementPtr tryVideoSink(const char *sink);
    QGst::ElementPtr constructVideoSink();

private Q_SLOTS:
    void toggleDtmf(bool checked);
    void toggleMute(bool checked);
    void hangup();
    void hold();
    void holdOperationFinished(Tp::PendingOperation *operation);
    void onHoldStatusChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);

    void toggleFullScreen();
    void exitFullScreen();


protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void hideEvent(QHideEvent *event);
    virtual void showEvent(QShowEvent *event);

private:
    struct Private;
    Private *const d;


private:
    SystemTrayIcon* systemtrayicon;
    void setupSystemTray();

};

#endif
