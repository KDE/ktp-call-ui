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
#include "calllog.h"
#include <QtGui/QPlainTextEdit>
#include <KLocalizedString>
#include <KDebug>

CallLog::CallLog(QPlainTextEdit *logView, QObject *parent)
    : QObject(parent), m_logView(logView)
{
    qRegisterMetaType<CallLog::LogType>();
}

void CallLog::logMessage(CallLog::LogType type, const QString & message)
{
    QString modifiedMessage;
    switch(type) {
    case Information:
        modifiedMessage = message;
        break;
    case Warning:
        modifiedMessage = i18n("Warning: %1", message);
        break;
    case Error:
        modifiedMessage = i18n("Error: %1", message);
        break;
    default:
        Q_ASSERT(false);
    }

    if ( m_logView ) {
        m_logView->appendPlainText(modifiedMessage);
        if ( type == Error ) {
            emit notifyUser();
        }
    } else {
        kDebug() << "LOG:" << modifiedMessage;
    }
}

#include "calllog.moc"
