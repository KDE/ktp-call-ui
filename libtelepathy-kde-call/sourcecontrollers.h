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
#ifndef SOURCECONTROLLERS_H
#define SOURCECONTROLLERS_H

#include "volumecontroller.h"

class CallContentHandlerPrivate;
class BaseSourceControllerPrivate;
class AudioSourceControllerPrivate;
class VideoSourceControllerPrivate;


class BaseSourceController : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(BaseSourceController)
    Q_PROPERTY(bool sourceEnabled READ sourceEnabled WRITE setSourceEnabled
                                  NOTIFY sourceEnabledChanged)
public:
    /*! This returns a new 'src' pad from the source bin that can be
     * connected to any sink that needs to get the data of the source.
     * This is useful for connecting a video preview sink on video contents
     * or for connecting an encoder and recording the call.
     * \note The pad must be released with releaseSrcPad() when
     * it is no longer needed.
     */
    QGst::PadPtr requestSrcPad();
    void releaseSrcPad(const QGst::PadPtr & pad);

    bool sourceEnabled() const;

public Q_SLOTS:
    void setSourceEnabled(bool enabled);

Q_SIGNALS:
    void sourceEnabledChanged(bool isEnabled);

protected:
    explicit BaseSourceController(BaseSourceControllerPrivate *d, QObject *parent = 0);
    virtual ~BaseSourceController();

    BaseSourceControllerPrivate * const d_ptr;
};


class AudioSourceController : public BaseSourceController
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(AudioSourceController)
    Q_PROPERTY(VolumeController* volumeController READ volumeController)
public:
    explicit AudioSourceController(const QGst::PipelinePtr & pipeline, QObject *parent = 0);

    VolumeController *volumeController() const;
    //TODO level monitor
};


class VideoSourceController : public BaseSourceController
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(VideoSourceController)
public:
    explicit VideoSourceController(const QGst::PipelinePtr & pipeline, QObject *parent = 0);

    //TODO colorbalance controls
};

#endif // SOURCECONTROLLERS_H
