/*
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

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
#ifndef CALLPARTICIPANT_H
#define CALLPARTICIPANT_H

#include <QtCore/QObject>
#include <TelepathyQt4/Types>
class CallChannelHandlerPrivate;
class ParticipantData;

class CallParticipant : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasAudioStream READ hasAudioStream)
    Q_PROPERTY(bool hasVideoStream READ hasVideoStream)
    Q_PROPERTY(bool mute READ isMuted WRITE setMuted)
    Q_PROPERTY(int volume READ volume WRITE setVolume)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness)
    Q_PROPERTY(int contrast READ contrast WRITE setContrast)
    Q_PROPERTY(int hue READ hue WRITE setHue)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation)
public:
    // the contact
    Tp::ContactPtr contact() const;
    bool isMyself() const;

    // the streams that we have
    bool hasAudioStream() const;
    bool hasVideoStream() const;

    // audio controls
    bool isMuted() const;
    void setMuted(bool mute);

    int volume() const;
    void setVolume(int volume);

    // video controls
    QWidget *videoWidget() const;

    int brightness() const;
    void setBrightness(int brightness);

    int contrast() const;
    void setContrast(int contrast);

    int hue() const;
    void setHue(int hue);

    int saturation() const;
    void setSaturation(int saturation);

Q_SIGNALS:
    void audioStreamAdded(CallParticipant *self);
    void audioStreamRemoved(CallParticipant *self);
    void videoStreamAdded(CallParticipant *self);
    void videoStreamRemoved(CallParticipant *self);

private:
    friend class CallChannelHandlerPrivate;

    CallParticipant(const QExplicitlySharedDataPointer<ParticipantData> & dd, QObject *parent = 0);
    virtual ~CallParticipant();
    Q_DISABLE_COPY(CallParticipant)

    QExplicitlySharedDataPointer<ParticipantData> d;
};

Q_DECLARE_METATYPE(CallParticipant*)

#endif // CALLPARTICIPANT_H
