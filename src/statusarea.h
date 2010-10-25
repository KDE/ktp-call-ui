/*
    Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
      @author George Kiagiadakis <george.kiagiadakis@collabora.co.uk>

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
#ifndef STATUSAREA_H
#define STATUSAREA_H

#include <QtCore/QObject>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QWeakPointer>
#include <QtGui/QLabel>
#include <KStatusBar>

class StatusArea : public QObject
{
    Q_OBJECT
public:
    StatusArea(KStatusBar *statusBar);

public Q_SLOTS:
    void startDurationTimer();
    void stopDurationTimer();
    void setStatusMessage(const QString & message);
    void showAudioStatusIcon(bool show);
    void showVideoStatusIcon(bool show);

private Q_SLOTS:
    void onCallDurationTimerTimeout();

private:
    KStatusBar *m_statusBar;
    QTime m_callDuration;
    QTimer m_callDurationTimer;
    QLabel *m_statusLabel;
    QWeakPointer<QWidget> m_audioStatusIcon;
    QWeakPointer<QWidget> m_videoStatusIcon;
};

#endif // STATUSAREA_H
