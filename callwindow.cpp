/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "callwindow.h"
#include <QtCore/QMetaObject>
#include <QtGui/QCloseEvent>
#include <QtGui/QLabel>
#include <KDebug>
#include <KLocalizedString>
#include <KPluginLoader>
#include <KPluginFactory>
#include <KParts/Part>

struct CallWindow::Private
{
    KPluginLoader *loader;
    KParts::Part *part;
};

/*! This constructor is used to make an outgoing call to the specified \a contact */
CallWindow::CallWindow(Tp::ContactPtr contact)
    : KParts::MainWindow(), d(new Private)
{
    init();
    if ( d->part ) {
        QMetaObject::invokeMethod(d->part, "callContact", Q_ARG(Tp::ContactPtr, contact));
    }
}

/*! This constructor is used to handle an incoming call, in which case
 * the specified \a channel must be ready and the call must have been accepted.
 */
CallWindow::CallWindow(Tp::StreamedMediaChannelPtr channel)
    : KParts::MainWindow(), d(new Private)
{
    init();
    if ( d->part ) {
        QMetaObject::invokeMethod(d->part, "handleStreamedMediaChannel",
                                  Q_ARG(Tp::StreamedMediaChannelPtr, channel));
    }
}

CallWindow::~CallWindow()
{
    kDebug() << "Deleting CallWindow";
    delete d;
}

void CallWindow::init()
{
    d->part = NULL;
    d->loader = new KPluginLoader("kcall_callwindowpart", KGlobal::mainComponent(), this);
    KPluginFactory *factory = d->loader->factory();
    if ( factory ) {
        d->part = factory->create<KParts::Part>(this, this);
        //version check
        int index = d->part->metaObject()->indexOfClassInfo("Interface version");
        const char *version = d->part->metaObject()->classInfo(index).value();
        if ( !version || version[0] != '0' ) {
            kWarning() << "Incompatible plugin version loaded";
            d->part->deleteLater();
            d->part = NULL;
        }
    }

    if ( d->part ) {
        setCentralWidget(d->part->widget());
    } else {
        QLabel *label = new QLabel(i18nc("@info:status", "Could not load call window part. "
                                                         "Please check your installation."), this);
        label->setWordWrap(true);
        setCentralWidget(label);
    }

    setAutoSaveSettings(QLatin1String("CallWindow"));
    setupGUI(QSize(320, 260), ToolBar | Keys | StatusBar | Save, QLatin1String("callwindowui.rc"));
    if ( d->part ) {
        createGUI(d->part);
    } else {
        KXmlGuiWindow::createGUI(QLatin1String("callwindowui.rc"));
    }
}

void CallWindow::onCallEnded(bool hasError)
{
}

void CallWindow::closeEvent(QCloseEvent *event)
{
    if ( d->part ) {
        bool canClose;
        if ( QMetaObject::invokeMethod(d->part, "requestClose", Q_RETURN_ARG(bool, canClose)) ) {
            if ( !canClose ) {
                kDebug() << "Ignoring close event";
                connect(d->part, SIGNAL(callEnded(bool)), SLOT(close()));
                event->ignore();
            }
        }
    }
    KParts::MainWindow::closeEvent(event);
}

#include "callwindow.moc"
