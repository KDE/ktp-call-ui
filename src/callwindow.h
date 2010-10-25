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
#ifndef CALLWINDOW_H
#define CALLWINDOW_H

#include "calllog.h"
#include <TelepathyQt4/StreamedMediaChannel>
#include <KXmlGuiWindow>
class CallParticipant;

class CallWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    CallWindow(const Tp::StreamedMediaChannelPtr & channel);
    virtual ~CallWindow();

private:
    void setupActions();
    void setupUi();
    void disableUi();

    enum State { Connecting, Connected, Disconnected };

private slots:
    void setState(State state);

    void onSendVideoStateChanged(bool enabled);
    void toggleSendVideo();

    void onParticipantJoined(CallParticipant *participant);
    void onParticipantLeft(CallParticipant *participant);
    void onParticipantAudioStreamAdded(CallParticipant *participant);
    void onParticipantAudioStreamRemoved(CallParticipant *participant);
    void onParticipantVideoStreamAdded(CallParticipant *participant);
    void onParticipantVideoStreamRemoved(CallParticipant *participant);
    void logErrorMessage(const QString & error);

    void onCallEnded(const QString & message);

protected:
    virtual void closeEvent(QCloseEvent *event);

private:
    struct Private;
    Private *const d;
};

#endif
