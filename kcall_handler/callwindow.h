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
#ifndef CALLWINDOW_H
#define CALLWINDOW_H

#include "channelhandler.h"
#include <KXmlGuiWindow>

class CallWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    CallWindow(Tp::StreamedMediaChannelPtr channel);
    virtual ~CallWindow();

private:
    void setupActions();
    void setupUi();
    void disableUi();

private slots:
    void setState(ChannelHandler::State state);
    void setStatus(const QString & msg);
    void onMediaHandlerCreated(AbstractMediaHandler *handler);
    void onAudioInputDeviceCreated(QObject *control);
    void onAudioInputDeviceDestroyed();
    void onAudioOutputDeviceCreated(QObject *control);
    void onAudioOutputDeviceDestroyed();
    void onVideoInputDeviceCreated(QWidget *videoWidget);
    void onVideoOutputWidgetCreated(QWidget *widget, uint id);
    void onCloseVideoOutputWidget(uint id);
    void onGroupMembersModelCreated(GroupMembersModel *model);
    void onDtmfHandlerCreated(DtmfHandler *handler);
    void onCallDurationTimerTimeout();

protected:
    virtual void closeEvent(QCloseEvent *event);

private:
    struct Private;
    Private *const d;
};

#endif
