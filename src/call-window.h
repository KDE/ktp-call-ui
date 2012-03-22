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
#ifndef CALL_WINDOW_H
#define CALL_WINDOW_H

#include <TelepathyQt/CallChannel>
#include <KXmlGuiWindow>
class CallContentHandler;

class CallWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    CallWindow(const Tp::CallChannelPtr & channel);
    virtual ~CallWindow();

private:
    void setupActions();

private Q_SLOTS:
    void setState(Tp::CallState state);
    void onCallEnded();
    void onAudioContentAdded(CallContentHandler *contentHandler);
    void onAudioContentRemoved(CallContentHandler *contentHandler);
    void onVideoContentAdded(CallContentHandler *contentHandler);
    void onVideoContentRemoved(CallContentHandler *contentHandler);
    void hangup();

protected:
    virtual void closeEvent(QCloseEvent *event);

private:
    struct Private;
    Private *const d;
};

#endif
