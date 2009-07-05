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
#ifndef _KGSTDEVICES_ELEMENTFACTORY_P_H
#define _KGSTDEVICES_ELEMENTFACTORY_P_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>

namespace KGstDevices {

enum ElementCategory {
    AudioInput,
    AudioOutput,
    VideoInput,
    VideoOutput
};

struct Property
{
    Property(const QByteArray & name, const QString & i18nName);
    QByteArray name;
    QString i18nName;
};

struct ElementDescription
{
    ElementCategory category;
    QByteArray name;
    QString i18nName;
    QList<Property> properties;
};

typedef QSharedPointer<const ElementDescription> ElementDescriptionPtr;

namespace ElementFactory
{
    ElementDescriptionPtr elementDescriptionByName(const QByteArray & name);
    QList<ElementDescriptionPtr> elementDescriptionsByCategory(ElementCategory category);
}

}

#endif
