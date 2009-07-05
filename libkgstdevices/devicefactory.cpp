/*
    This file is based on code from TelepathyQt4 and Phonon.

    Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "elementfactory_p.h"
#include "mediadevices.h"
#include <KDebug>
#include <KLocalizedString>
#include <gst/gst.h>
#include <gst/interfaces/propertyprobe.h>

namespace KGstDevices {

struct DeviceFactory::Private
{
    QStringList errorLog;
    KConfigGroup config;
    AudioDevice *audioInputDevice;
    AudioDevice *audioOutputDevice;
};

DeviceFactory::DeviceFactory(const KConfigGroup & group)
    : d(new Private)
{
    d->config = group;
}

DeviceFactory::~DeviceFactory()
{
    delete d->audioInputDevice;
    delete d->audioOutputDevice;
    delete d;
}

bool DeviceFactory::createAudioInputDevice()
{
    QByteArray mainElementName("autodetect");
    if ( d->config.isValid() ) {
        mainElementName = d->config.readEntry("AudioSource", "autodetect").toAscii();
    }
    ElementDescriptionPtr elementDescription = ElementFactory::elementDescriptionByName(mainElementName);
    GstElement *element = createAudioSource(elementDescription);
    if ( !element ) {
        d->audioInputDevice = NULL;
        return false;
    } else {
        return (d->audioInputDevice = AudioDevice::createAudioInputDevice(element)) != NULL;
    }
}

bool DeviceFactory::createAudioOutputDevice()
{
    QByteArray mainElementName("autodetect");
    if ( d->config.isValid() ) {
        mainElementName = d->config.readEntry("AudioSink", "autodetect").toAscii();
    }
    ElementDescriptionPtr elementDescription = ElementFactory::elementDescriptionByName(mainElementName);
    GstElement *element = createAudioSink(elementDescription);
    if ( !element ) {
        d->audioOutputDevice = NULL;
        return false;
    } else {
        return (d->audioOutputDevice = AudioDevice::createAudioOutputDevice(element)) != NULL;
    }
}

AudioDevice *DeviceFactory::audioInputDevice() const
{
    return d->audioInputDevice;
}

AudioDevice *DeviceFactory::audioOutputDevice() const
{
    return d->audioOutputDevice;
}

bool DeviceFactory::hasError() const
{
    return !d->errorLog.isEmpty();
}

QStringList DeviceFactory::errorLog() const
{
    return d->errorLog;
}

void DeviceFactory::clearLog()
{
    d->errorLog.clear();
}

static inline GValue *gValueFromQVariant(const QVariant & variant, GType gType)
{
    GValue *v = g_slice_new0(GValue);
    g_value_init(v, gType);

    switch (gType) {
    case G_TYPE_BOOLEAN:
        g_value_set_boolean(v, variant.value<gboolean>());
        break;
    case G_TYPE_CHAR:
        g_value_set_char(v, variant.value<char>());
        break;
    // skip G_TYPE_BOXED
    case G_TYPE_DOUBLE:
        g_value_set_double(v, variant.value<gdouble>());
        break;
    // skip G_TYPE_ENUM
    // skip G_TYPE_FLAGS
    case G_TYPE_FLOAT:
        g_value_set_float(v, variant.value<gfloat>());
        break;
    // skip G_TYPE_GTYPE
    case G_TYPE_INT:
        g_value_set_int(v, variant.value<gint>());
        break;
    case G_TYPE_INT64:
        g_value_set_int64(v, variant.value<gint64>());
        break;
    case G_TYPE_LONG:
        g_value_set_long(v, variant.value<glong>());
        break;
    // skip G_TYPE_OBJECT
    // skip G_TYPE_PARAM
    // skip G_TYPE_POINTER:
    case G_TYPE_STRING:
        g_value_set_string(v, variant.value<QString>().toLocal8Bit().constData());
        break;
    case G_TYPE_UCHAR:
        g_value_set_uchar(v, variant.value<guchar>());
        break;
    case G_TYPE_UINT:
        g_value_set_uint(v, variant.value<guint>());
        break;
    case G_TYPE_UINT64:
        g_value_set_uint64(v, variant.value<guint64>());
        break;
    case G_TYPE_ULONG:
        g_value_set_ulong(v, variant.value<gulong>());
        break;
    // skip custom types
    default:
        g_slice_free(GValue, v);
        return 0;
    }

    return v;
}

/**
 * Probes a gstElement for a list of settable string-property values
 *
 * @return a QStringList containing a list of allwed string values for the given
 *           element
 */
QList<QByteArray> extractProperties(GstElement *elem, const QByteArray &value)
{
    Q_ASSERT(elem);
    QList<QByteArray> list;

    if (GST_IS_PROPERTY_PROBE(elem)) {
        GstPropertyProbe *probe = GST_PROPERTY_PROBE(elem);
        const GParamSpec *devspec = 0;
        GValueArray *array = NULL;

        if ((devspec = gst_property_probe_get_property (probe, value))) {
            if ((array = gst_property_probe_probe_and_get_values (probe, devspec))) {
                for (unsigned int device = 0; device < array->n_values; device++) {
                    GValue *deviceId = g_value_array_get_nth (array, device);
                    list.append(g_value_get_string(deviceId));
                }
            }
            if (array) {
                g_value_array_free (array);
            }
        }
    }
    return list;
}

bool DeviceFactory::canOpenAudioDevice(GstElement *element) const
{
    if (!element)
        return false;

    if (gst_element_set_state(element, GST_STATE_READY) == GST_STATE_CHANGE_SUCCESS)
        return true;

    const QList<QByteArray> &list = extractProperties(element, "device");
    foreach (const QByteArray &gstId, list) {
        g_object_set(G_OBJECT(element), "device", gstId.constData(), NULL);
        if (gst_element_set_state(element, GST_STATE_READY) == GST_STATE_CHANGE_SUCCESS) {
            return true;
        }
    }

    gst_element_set_state(element, GST_STATE_NULL);
    return false;
}

void DeviceFactory::loadDeviceSettings(GstElement *element, ElementDescriptionPtr description) const
{
    foreach(const Property & property, description->properties) {
        const QByteArray & name = property.name;
        QVariant variant = d->config.isValid() ? d->config.readEntry(name.constData()) : QVariant();
        if ( variant.isValid() ) {
            GParamSpec *paramSpec = g_object_class_find_property(G_OBJECT_GET_CLASS(element),
                                                                 name.constData());
            GValue *value = gValueFromQVariant(variant, G_PARAM_SPEC_VALUE_TYPE(paramSpec));
            if ( !value ) {
                kWarning() << "Could not convert QVariant (" << variant << ") to GValue"
                           << "for the property:" << name << ", using the GType:"
                           << G_PARAM_SPEC_VALUE_TYPE(paramSpec);
            } else {
                g_object_set_property(G_OBJECT(element), name.constData(), value);
                g_slice_free(GValue, value);
            }
        }
    }
}

GstElement *DeviceFactory::createAudioSource(ElementDescriptionPtr description)
{
    GstElement *element;
    if ( !description.isNull() && description->name != "autodetect" ) {
        element = gst_element_factory_make(description->name, NULL);
        if ( !element ) {
            kWarning() << "could not create audio input element:" << description->name;
            d->errorLog << i18n("The audio system \"%1\" is not available. "
                                "Falling back to autodetection.").arg(description->i18nName);
        } else {
            loadDeviceSettings(element, description);
            if ( !canOpenAudioDevice(element) ) {
                d->errorLog << i18n("Failed to open audio input device using the \"%1\" audio system. "
                                    "Falling back to autodetection.").arg(description->i18nName);
                gst_object_unref(element);
                element = NULL;
            } else {
                kDebug() << "Using" << description->name << "as audio source";
                return element;
            }
        }
    }

    QList<QByteArray> devicesToTry;

    //if we are in gnome, respect gnome settings
    if (!qgetenv("GNOME_DESKTOP_SESSION_ID").isEmpty()) {
        devicesToTry << "gconfaudiosrc";
    }
    devicesToTry << "alsasrc" << "autoaudiosrc" << "osssrc";

    foreach(const QByteArray & device, devicesToTry) {
        element = gst_element_factory_make(device, NULL);
        if ( element ) {
            if ( !canOpenAudioDevice(element) ) {
                gst_object_unref(element);
                element = NULL;
            } else {
                kDebug() << "Using" << device << "as audio source";
                return element;
            }
        }
    }

    kDebug() << "No audio source";
    d->errorLog << i18n("No suitable audio input system found. Audio input will not work.");
    return NULL;
}

GstElement *DeviceFactory::createAudioSink(ElementDescriptionPtr description)
{
    GstElement *element;
    if ( !description.isNull() && description->name != "autodetect" ) {
        element = gst_element_factory_make(description->name, NULL);
        if ( !element ) {
            kWarning() << "could not create audio output element:" << description->name;
            d->errorLog << i18n("The audio system \"%1\" is not available. "
                                "Falling back to autodetection.").arg(description->i18nName);
        } else {
            loadDeviceSettings(element, description);
            if ( !canOpenAudioDevice(element) ) {
                d->errorLog << i18n("Failed to open audio output device using the \"%1\" audio system. "
                                    "Falling back to autodetection.").arg(description->i18nName);
                gst_object_unref(element);
                element = NULL;
            } else {
                kDebug() << "Using" << description->name << "as audio sink";
                return element;
            }
        }
    }

    //if we are in gnome, respect gnome settings
    if (!qgetenv("GNOME_DESKTOP_SESSION_ID").isEmpty()) {
        element = gst_element_factory_make("gconfaudiosink", NULL);
        if ( element ) {
            g_object_set(G_OBJECT (element), "profile", 2, (const char*)NULL); // 2 = 'chat'
            if ( !canOpenAudioDevice(element) ) {
                gst_object_unref(element);
                element = NULL;
            } else {
                kDebug() << "Using gconfaudiosink as audio sink";
                return element;
            }
        }
    }

    QList<QByteArray> devicesToTry;
    devicesToTry << "alsasink" << "autoaudiosink" << "osssink";
    foreach(const QByteArray & device, devicesToTry) {
        element = gst_element_factory_make(device, NULL);
        if ( element ) {
            if ( !canOpenAudioDevice(element) ) {
                gst_object_unref(element);
                element = NULL;
            } else {
                kDebug() << "Using" << device << "as audio sink";
                return element;
            }
        }
    }

    kDebug() << "No audio sink";
    d->errorLog << i18n("No suitable audio output system found. Audio output will not work.");
    return NULL;
}

}
