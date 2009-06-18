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
#ifndef CHANNELHANDLER_H
#define CHANNELHANDLER_H

#include <QtCore/QObject>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Constants>
namespace Tp { class PendingOperation; class DBusProxy; }

class ChannelHandler : public QObject
{
    Q_OBJECT
public:
    ChannelHandler(QObject *parent = 0);

    enum State { Connecting, Ringing, InCall, HangingUp, Disconnected, Error };
    void handleChannel(Tp::StreamedMediaChannelPtr channel);
    bool requestClose();

public slots:
    void hangupCall();

signals:
    void stateChanged(ChannelHandler::State newState);
    void callEnded(bool withError);

private:
    void setState(State s);

private slots:
    void onChannelReady(Tp::PendingOperation *op);
    void onChannelInvalidated(Tp::DBusProxy *proxy, const QString &errorName,
                              const QString &errorMessage);
    void onStreamAdded(const Tp::MediaStreamPtr & stream);
    void onStreamRemoved(const Tp::MediaStreamPtr & stream);
    void onStreamDirectionChanged(const Tp::MediaStreamPtr & stream,
                                  Tp::MediaStreamDirection direction,
                                  Tp::MediaStreamPendingSend pendingSend);
    void onStreamStateChanged(const Tp::MediaStreamPtr & stream, Tp::MediaStreamState state);
    void onChannelClosed(Tp::PendingOperation *op);

private:
    Tp::StreamedMediaChannelPtr m_channel;
    State m_state;
};

#endif
