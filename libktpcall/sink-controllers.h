/*
    Copyright (C) 2011 Collabora Ltd. <info@collabora.co.uk>
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
#ifndef SINK_CONTROLLERS_H
#define SINK_CONTROLLERS_H

#include "volume-controller.h"
#include <TelepathyQt/Contact>

class BaseSinkControllerPrivate;
class AudioSinkControllerPrivate;
class VideoSinkControllerPrivate;
class BaseSinkManager;
class AudioSinkManager;
class VideoSinkManager;

//FIXME shouldn't this be in tp-qt4?
Q_DECLARE_METATYPE(Tp::ContactPtr)


class BaseSinkController : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(BaseSinkController)
    Q_PROPERTY(Tp::ContactPtr contact READ contact)
public:
    Tp::ContactPtr contact() const;

protected:
    friend class BaseSinkManager;
    explicit BaseSinkController(BaseSinkControllerPrivate *d, QObject *parent = 0);
    virtual ~BaseSinkController();

    BaseSinkControllerPrivate * const d_ptr;
};


class AudioSinkController : public BaseSinkController
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(AudioSinkController)
    Q_PROPERTY(VolumeController* volumeController READ volumeController)
public:
    VolumeController *volumeController() const;
    //TODO level monitor

private:
    friend class AudioSinkManager;
    explicit AudioSinkController(AudioSinkControllerPrivate *d, QObject *parent = 0);
};


class VideoSinkController : public BaseSinkController
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(VideoSinkController)
public:
    QGst::PadPtr requestSrcPad();
    void releaseSrcPad(const QGst::PadPtr & pad);

    //TODO colorbalance control

private:
    friend class VideoSinkManager;
    explicit VideoSinkController(VideoSinkControllerPrivate *d, QObject *parent = 0);
};

#endif //SINK_CONTROLLERS_H
