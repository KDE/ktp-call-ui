/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ABSTRACTMEDIAHANDLER_H
#define ABSTRACTMEDIAHANDLER_H

#include <QtCore/QObject>
#include <TelepathyQt4/StreamedMediaChannel>

class AbstractAudioDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume)

public:
    virtual ~AbstractAudioDevice() {}

    virtual bool isMuted() const = 0;
    virtual qreal volume() const = 0;

public Q_SLOTS:
    virtual void setMuted(bool mute) = 0;
    virtual void setVolume(qreal volume) = 0;

protected:
    AbstractAudioDevice(QObject *parent = 0);
};

class AbstractMediaHandler : public QObject
{
    Q_OBJECT
    Q_ENUMS(Capability)
    Q_FLAGS(Capabilities)
public:
    enum Capability {
        None = 0x0,
        SendAudio = 0x1,
        ReceiveAudio = 0x2,
        SendVideo = 0x4,
        ReceiveVideo = 0x8
    };
    Q_DECLARE_FLAGS(Capabilities, Capability);

    enum Status {
        Disconnected,
        Connecting,
        Connected
    };

    static AbstractMediaHandler *create(const Tp::StreamedMediaChannelPtr & channel,
                                        QObject *parent = 0);

    Status status() const;
    virtual Capabilities capabilities() const = 0;
    virtual AbstractAudioDevice *audioInputDevice() const = 0;
    virtual AbstractAudioDevice *audioOutputDevice() const = 0;

Q_SIGNALS:
    void statusChanged(AbstractMediaHandler::Status newStatus);

protected:
    AbstractMediaHandler(QObject *parent = 0);

protected Q_SLOTS:
    void setStatus(AbstractMediaHandler::Status s);

private:
    Status m_status;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AbstractMediaHandler::Capabilities)

#endif
