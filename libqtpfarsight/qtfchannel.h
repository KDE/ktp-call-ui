/*
    Copyright (C) 2009-2010  George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#ifndef QTFCHANNEL_H
#define QTFCHANNEL_H

#include <QGst/Pad>
#include <QGst/Element>
#include <TelepathyQt4/StreamedMediaChannel>

/** This is a wrapper class around TfChannel and TfStream that provides a saner Qt-ish API.
 * The API is highly asynchronous. Everything is done through signals. The caller needs to connect
 * to all these signals and react appropriately on them.
 */
class QTfChannel : public QObject
{
    Q_OBJECT
public:
    /** Constructs a QTfChannel that corresponds to the specified @a channel.
     * @a bus should be the GstBus of the pipeline.
     */
    QTfChannel(const Tp::StreamedMediaChannelPtr & channel,
               const QGst::BusPtr & bus, QObject *parent = 0);
    virtual ~QTfChannel();

    /** Sets the filename of the file where codec preferences should be read from.
     * The filename is passed to fs_codec_list_from_keyfile(). See the documentation of
     * that method (in the Farsight2 docs) for more information on the file format.
     */
    void setCodecsConfigFile(const QString & filename);

Q_SIGNALS:
    /** Emmited when the session starts. @a conference is the FsConference element
     * that should be added in the pipeline and set to PLAYING state.
     */
    void sessionCreated(QGst::ElementPtr conference);

    /** Emmited when the channel is closed. The pipeline should be stopped when
     * this signal is received.
     */
    void closed();


    /** Emmited when the audio input device should be initialized. @a suceess should
     * be set to true from the slot if the device was initialized properly and can be used.
     */
    void openAudioInputDevice(bool *success);

    /** Emmited when an audio sink pad is added on the conference.
     * @a sinkPad should be connected with the audio input device.
     */
    void audioSinkPadAdded(QGst::PadPtr sinkPad);

    /** Emmited when the audio input device can be closed */
    void closeAudioInputDevice();


    /** Emmited when the video input device should be initialized. @a suceess should
     * be set to true from the slot if the device was initialized properly and can be used.
     */
    void openVideoInputDevice(bool *success);

    /** Emmited when a video sink pad is added on the conference.
     * @a sinkPad should be connected with the video input device.
     */
    void videoSinkPadAdded(QGst::PadPtr sinkPad);

    /** Emmited when the video input device can be closed */
    void closeVideoInputDevice();


    /** Emmited when the audio output device should be initialized. @a suceess should
     * be set to true from the slot if the device was initialized properly and can be used.
     */
    void openAudioOutputDevice(bool *success);

    /** Emmited when an audio source pad is added on the conference.
     * @a srcPad should be connected with the audio output device.
     */
    void audioSrcPadAdded(QGst::PadPtr srcPad);

    /** Emmited when an audio source pad is removed from the conference.
     * @a srcPad should be disconnected from the audio output device.
     */
    void audioSrcPadRemoved(QGst::PadPtr srcPad);


    /** Emmited when the video output device should be initialized. @a suceess should
     * be set to true from the slot if the device was initialized properly and can be used.
     */
    void openVideoOutputDevice(bool *success);

    /** Emmited when a video source pad is added on the conference.
     * @a srcPad should be connected with a video widget.
     */
    void videoSrcPadAdded(QGst::PadPtr srcPad);

    /** Emmited when a video source pad is removed from the conference.
     * @a srcPad should be disconnected from the video widget.
     */
    void videoSrcPadRemoved(QGst::PadPtr srcPad);

private:
    class Private;
    friend class Private;
    Private *const d;
    Q_PRIVATE_SLOT(d, void _p_onAudioSrcPadAdded(QGst::PadPtr pad));
    Q_PRIVATE_SLOT(d, void _p_onVideoSrcPadAdded(QGst::PadPtr pad));
};

#endif
