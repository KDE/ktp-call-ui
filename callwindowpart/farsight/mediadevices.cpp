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
#include "mediadevices.h"
#include <KDebug>

namespace Farsight {

AudioDevice::AudioDevice(QObject *parent)
    : AbstractAudioDevice(parent),
      m_bin(NULL),
      m_volumeElement(NULL)
{
}

AudioDevice::~AudioDevice()
{
    if (m_bin) {
        gst_element_set_state(m_bin, GST_STATE_NULL);
        g_object_unref(m_bin);
        m_bin = NULL;
    }
}

GstElement *AudioDevice::bin() const
{
    return m_bin;
}

bool AudioDevice::isMuted() const
{
    if (!m_volumeElement) {
        kWarning() << "No volume control available on the device. Returning dummy value";
        return false;
    }

    gboolean result;
    g_object_get(GST_OBJECT(m_volumeElement), "mute", &result, NULL);
    return static_cast<bool>(result);
}

qreal AudioDevice::volume() const
{
    if (!m_volumeElement) {
        kWarning() << "No volume control available on the device. Returning dummy value";
        return qreal(0.0);
    }

    qreal result;
    g_object_get(GST_OBJECT(m_volumeElement), "volume", &result, NULL);
    return result;
}

void AudioDevice::setMuted(bool mute)
{
    if (!m_volumeElement) {
        kWarning() << "No volume control available on the device. Ignoring mute request";
        return;
    }

    g_object_set(GST_OBJECT(m_volumeElement), "mute", gboolean(mute), NULL);
}

void AudioDevice::setVolume(qreal volume)
{
    if (!m_volumeElement) {
        kWarning() << "No volume control available on the device. Ignoring volume change request";
        return;
    }

    g_object_set(GST_OBJECT(m_volumeElement), "volume", qBound<qreal>(0.0, volume, 10.0), NULL);
}


AudioDevice *AudioDevice::createInputDevice(QObject *parent)
{
    AudioDevice *device = new AudioDevice(parent);
    device->m_bin = gst_bin_new("audio-input-bin");
    GstElement *audioSrc = gst_element_factory_make("autoaudiosrc", NULL);
    //device->m_volumeElement = gst_element_factory_make("volume", "input_volume");

    if ( !audioSrc /*|| !device->m_volumeElement*/ ) {
        kError() << "Failed to initialize audio input device";
        if ( !audioSrc ) {
            kDebug() << "Audio source is NULL";
        }
        /*
        if ( !device->m_volumeElement ) {
            kDebug() << "Volume contoller is NULL";
        }
        */
        delete device;
        return NULL;
    }

    gst_bin_add_many(GST_BIN(device->m_bin), audioSrc, /*device->m_volumeElement,*/ NULL);
    //gst_element_link(audioSrc, device->m_volumeElement);
    GstPad *src = gst_element_get_static_pad(audioSrc /*device->m_volumeElement*/, "src");
    GstPad *ghost = gst_ghost_pad_new("src", src);
    Q_ASSERT(ghost);
    gst_element_add_pad(GST_ELEMENT(device->m_bin), ghost);
    gst_object_unref(src);
    gst_object_ref(device->m_bin);
    gst_object_sink(device->m_bin);

    //TODO load defaults from settings
    //device->setMuted(false);
    //device->setVolume(10.0);
    return device;
}

AudioDevice *AudioDevice::createOutputDevice(QObject *parent)
{
    AudioDevice *device = new AudioDevice(parent);
    device->m_bin = gst_bin_new("audio-output-bin");
    GstElement *resample = gst_element_factory_make("audioresample", NULL);
    device->m_volumeElement = gst_element_factory_make("volume", "output_volume");
    GstElement *audioSink = gst_element_factory_make("autoaudiosink", NULL);

    if ( !audioSink || !device->m_volumeElement || !resample ) {
        kError() << "Failed to initialize audio output device";
        if ( !audioSink ) {
            kDebug() << "Audio sink is NULL";
        }
        if ( !device->m_volumeElement ) {
            kDebug() << "Volume contoller is NULL";
        }
        if ( !resample ) {
            kDebug() << "Resampling element is NULL";
        }
        delete device;
        return NULL;
    }

    gst_bin_add_many(GST_BIN(device->m_bin), resample, device->m_volumeElement, audioSink, NULL);
    gst_element_link_many(resample, device->m_volumeElement, audioSink, NULL);
    GstPad *sink = gst_element_get_static_pad(resample, "sink");
    GstPad *ghost = gst_ghost_pad_new("sink", sink);
    Q_ASSERT(ghost);
    gst_element_add_pad(GST_ELEMENT(device->m_bin), ghost);
    gst_object_unref(sink);
    gst_object_ref(device->m_bin);
    gst_object_sink(device->m_bin);

    //TODO load defaults from settings
    device->setMuted(false);
    device->setVolume(10.0);
    return device;
}

} //namespace Farsight

#include "mediadevices.moc"
