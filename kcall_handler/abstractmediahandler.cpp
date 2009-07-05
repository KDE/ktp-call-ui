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
#include "farsight/mediahandler.h"

AbstractMediaHandler::AbstractMediaHandler(QObject *parent)
    : QObject(parent), m_status(Disconnected)
{
}

//static
AbstractMediaHandler *AbstractMediaHandler::create(const Tp::StreamedMediaChannelPtr & channel,
                                                   QObject *parent)
{
    return new Farsight::MediaHandler(channel, parent);
}

AbstractMediaHandler::Status AbstractMediaHandler::status() const
{
    return m_status;
}

void AbstractMediaHandler::setStatus(AbstractMediaHandler::Status s)
{
    if ( s != m_status ) {
        m_status = s;
        emit statusChanged(s);
    }
}

#include "abstractmediahandler.moc"
