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
#ifndef CALLLOG_H
#define CALLLOG_H

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QMetaType>
class QPlainTextEdit;

class CallLog : public QObject
{
    Q_OBJECT
    Q_ENUMS(LogType)
public:
    enum LogType {
        Information,
        Warning,
        Error
    };

    CallLog(QPlainTextEdit *logView, QObject *parent = 0);

public slots:
    void logMessage(CallLog::LogType type, const QString & message);

signals:
    void notifyUser();

private:
    QPointer<QPlainTextEdit> m_logView;
};

Q_DECLARE_METATYPE(CallLog::LogType);

#endif
