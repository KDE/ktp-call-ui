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
#include "callwindowpart_p.h"
#include <QtGui/QStackedWidget>
#include <QtGui/QLabel>
#include <KAboutData>
#include <KPluginFactory>

static KAboutData createAboutData()
{
    KAboutData aboutData("kcall_callwindowpart", "kcall", ki18n("Call Window Part"), "0.1",
                          ki18n("A kpart for making calls using the telepathy framework."),
                          KAboutData::License_LGPL,
                          ki18n("(C) 2009, George Kiagiadakis"));
    aboutData.addAuthor(ki18nc("@info:credit", "George Kiagiadakis"),
                        ki18n("Author"), "kiagiadakis.george@gmail.com");
    return aboutData;
}

K_PLUGIN_FACTORY(CallWindowPartFactory,
                 registerPlugin<CallWindowPart>();
                )
K_EXPORT_PLUGIN(CallWindowPartFactory(createAboutData()));


CallWindowPart::CallWindowPart(QWidget *parentWidget, QObject *parent, const QVariantList &args)
    : KParts::Part(parent), d_ptr(new CallWindowPartPrivate(this))
{
    Q_UNUSED(args);
    setComponentData(CallWindowPartFactory::componentData());

    Q_D(CallWindowPart);
    d->videoStackedWidget = new QStackedWidget(parentWidget);
    d->videoPlaceHolder = new QLabel;
    d->videoStackedWidget->addWidget(d->videoPlaceHolder);
    d->setupActions();
    setWidget(d->videoStackedWidget);
}

void CallWindowPart::callContact(Tp::ContactPtr contact)
{
    emit setWindowCaption(contact->alias());
    Q_D(CallWindowPart);
    d->callContact(contact);
}

void CallWindowPart::handleStreamedMediaChannel(Tp::StreamedMediaChannelPtr channel)
{
    Q_D(CallWindowPart);
    d->handleChannel(channel);
}

bool CallWindowPart::requestClose()
{
    Q_D(CallWindowPart);
    return d->requestClose();
}

#include "callwindowpart.moc"
