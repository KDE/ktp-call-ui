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
#include "elementfactory_p.h"
#include <QtCore/QHash>
#include <KGlobal>
#include <KLocalizedString>

namespace KGstDevices {

Property::Property(const QByteArray & name, const QString & i18nName)
    : name(name), i18nName(i18nName)
{
}

class ElementFactoryPrivate
{
public:
    ElementFactoryPrivate();
    QHash<QByteArray, ElementDescriptionPtr> m_elements;

private:
    void registerElement(ElementCategory category, const QByteArray & name, const QString & i18nName,
                         const QList<Property> & properties = QList<Property>());
};

ElementFactoryPrivate::ElementFactoryPrivate()
{
    //audio input
    registerElement(AudioInput, "autodetect", i18n("Auto detect"));
    registerElement(AudioInput, "alsasrc", i18n("Alsa"),
                    QList<Property>() << Property("device", i18n("Alsa device")));
    registerElement(AudioInput, "osssrc", i18n("OSS"),
                    QList<Property>() << Property("device", i18n("OSS device")));
    registerElement(AudioInput, "jackaudiosrc", i18n("Jack"),
                    QList<Property>() << Property("server", i18n("Jack audio server")));
    registerElement(AudioInput, "pulsesrc", i18n("Pulse audio"),
                    QList<Property>() << Property("device", i18n("Pulse audio device"))
                                      << Property("server", i18n("Pulse audio server")));

    //audio output
    registerElement(AudioOutput, "autodetect", i18n("Auto detect"));
    registerElement(AudioOutput, "alsasink", i18n("Alsa"),
                    QList<Property>() << Property("device", i18n("Alsa device")));
    registerElement(AudioOutput, "osssink", i18n("OSS"),
                    QList<Property>() << Property("device", i18n("OSS device")));
    registerElement(AudioOutput, "jackaudiosink", i18n("Jack"),
                    QList<Property>() << Property("server", i18n("Jack audio server")));
    registerElement(AudioOutput, "pulsesink", i18n("Pulse audio"),
                    QList<Property>() << Property("device", i18n("Pulse audio device"))
                                      << Property("server", i18n("Pulse audio server")));
}

void ElementFactoryPrivate::registerElement(ElementCategory category, const QByteArray & name,
                                            const QString & i18nName,
                                            const QList<Property> & properties)
{
    ElementDescription *desc = new ElementDescription;
    desc->category = category;
    desc->name = name;
    desc->i18nName = i18nName;
    desc->properties = properties;
    m_elements.insert(name, ElementDescriptionPtr(desc));
}

K_GLOBAL_STATIC(ElementFactoryPrivate, s_priv);

namespace ElementFactory {

ElementDescriptionPtr elementDescriptionByName(const QByteArray & name)
{
    return s_priv->m_elements.value(name);
}


QList<ElementDescriptionPtr> elementDescriptionsByCategory(ElementCategory category)
{
    QList<ElementDescriptionPtr> list;
    QHashIterator<QByteArray, ElementDescriptionPtr> i(s_priv->m_elements);
    while(i.hasNext()) {
        i.next();
        if ( i.value()->category == category ) {
            list.append(i.value());
        }
    }
    return list;
}

}
}
