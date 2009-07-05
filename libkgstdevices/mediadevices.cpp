/*
    This file is based on code from the TelepathyQt4 library.

    Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
    Copyright (C) 2009 Nokia Corporation
    Copyright (C) 2009 George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#include <gst/gst.h>

namespace KGstDevices {

struct Device::Private
{
    GstElement *bin;
    GstElement *mainElement;
};

Device::Device(GstElement *bin, GstElement *mainElement)
    : d(new Private)
{
    d->bin = bin;
    d->mainElement = mainElement;
}

Device::~Device()
{
    if (d->bin) {
        gst_element_set_state(d->bin, GST_STATE_NULL);
        g_object_unref(d->bin);
        d->bin = NULL;
    }
    delete d;
}

GstElement *Device::bin() const
{
    return d->bin;
}

GstElement *Device::mainElement() const
{
    return d->mainElement;
}


struct AudioDevice::Private
{
    GstElement *volumeElement;
};

AudioDevice::AudioDevice(GstElement *bin, GstElement *mainElement, GstElement *volumeElement)
    : Device(bin, mainElement), d(new Private)
{
    d->volumeElement = volumeElement;
}

AudioDevice::~AudioDevice()
{
    delete d;
}

bool AudioDevice::volumeControlIsAvailable() const
{
    return (d->volumeElement != NULL);
}

bool AudioDevice::isMuted() const
{
    if (!d->volumeElement) {
        kWarning() << "No volume control available on the device. Returning dummy value";
        return false;
    }

    gboolean result;
    g_object_get(GST_OBJECT(d->volumeElement), "mute", &result, NULL);
    return static_cast<bool>(result);
}

void AudioDevice::setMuted(bool mute)
{
    if (!d->volumeElement) {
        kWarning() << "No volume control available on the device. Ignoring mute request";
        return;
    }

    g_object_set(GST_OBJECT(d->volumeElement), "mute", gboolean(mute), NULL);
}

int AudioDevice::minVolume() const
{
    return 0;
}

int AudioDevice::maxVolume() const
{
    return 1000;
}

int AudioDevice::volume() const
{
    if (!d->volumeElement) {
        kWarning() << "No volume control available on the device. Returning dummy value";
        return 0;
    }

    double result;
    g_object_get(GST_OBJECT(d->volumeElement), "volume", &result, NULL);
    return static_cast<int>(result*100);
}

void AudioDevice::setVolume(int volume)
{
    if (!d->volumeElement) {
        kWarning() << "No volume control available on the device. Ignoring volume change request";
        return;
    }

    g_object_set(GST_OBJECT(d->volumeElement), "volume",
                 double(qBound<int>(0, volume, 1000)) / double(100.0), NULL);
}

AudioDevice *AudioDevice::createAudioInputDevice(GstElement *mainElement)
{
    GstElement *bin, *volumeElement;

    bin = gst_bin_new(NULL);
    Q_ASSERT(bin);
    volumeElement = gst_element_factory_make("volume", NULL);
    if ( !volumeElement ) {
        kWarning() << "Could not create volume element";
    }

    gst_bin_add_many(GST_BIN(bin), mainElement, volumeElement, NULL);
    if ( volumeElement ) {
        gst_element_link(mainElement, volumeElement);
    }

    GstPad *src = gst_element_get_static_pad(volumeElement ? volumeElement : mainElement, "src");
    GstPad *ghost = gst_ghost_pad_new("src", src);
    Q_ASSERT(ghost);

    gst_element_add_pad(GST_ELEMENT(bin), ghost);
    gst_object_unref(src);
    gst_object_ref(bin);
    gst_object_sink(bin);

    return new AudioDevice(bin, mainElement, volumeElement);
}

AudioDevice *AudioDevice::createAudioOutputDevice(GstElement *mainElement)
{
    GstElement *bin, *volumeElement;

    GstElement *resample = gst_element_factory_make("audioresample", NULL);
    if ( !resample ) {
        kError() << "Could not create resample element";
        return NULL;
    }

    bin = gst_bin_new(NULL);
    Q_ASSERT(bin);
    volumeElement = gst_element_factory_make("volume", NULL);
    if ( !volumeElement ) {
        kWarning() << "Could not create volume element";
    }

    gst_bin_add_many(GST_BIN(bin), resample, mainElement, volumeElement, NULL);
    if ( volumeElement ) {
        gst_element_link_many(resample, volumeElement, mainElement, NULL);
    } else {
        gst_element_link_many(resample, mainElement, NULL);
    }

    GstPad *sink = gst_element_get_static_pad(resample, "sink");
    GstPad *ghost = gst_ghost_pad_new("sink", sink);
    Q_ASSERT(ghost);

    gst_element_add_pad(GST_ELEMENT(bin), ghost);
    gst_object_unref(sink);
    gst_object_ref(bin);
    gst_object_sink(bin);

    return new AudioDevice(bin, mainElement, volumeElement);
}

}
