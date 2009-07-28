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
#include "abstractmediahandler.h"
#include "farsight/farsightmediahandler.h"
#include <KDebug>

AbstractMediaHandler::AbstractMediaHandler(QObject *parent)
    : QObject(parent)
{
}

//static
AbstractMediaHandler *AbstractMediaHandler::create(const Tp::StreamedMediaChannelPtr & channel,
                                                   QObject *parent)
{
    kDebug() << "Creating farsight channel";
    return new FarsightMediaHandler(channel, parent);
}

#include "abstractmediahandler.moc"
