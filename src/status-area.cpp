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
#include "status-area.h"

#include <KIconLoader>

#include <QIcon>
#include <KDebug>

StatusArea::StatusArea(QStatusBar *statusBar)
    : QObject(statusBar), m_statusBar(statusBar)
{
    connect(&m_callDurationTimer, SIGNAL(timeout()), SLOT(onCallDurationTimerTimeout()));

    m_callDuration.setHMS(0, 0, 0);
    m_statusBar->showMessage(m_callDuration.toString(), 0);

    m_statusLabel = new QLabel;
    m_statusBar->addWidget(m_statusLabel);
}

void StatusArea::startDurationTimer()
{
    m_callDurationTimer.start(1000);
}

void StatusArea::stopDurationTimer()
{
    m_callDurationTimer.stop();
}

void StatusArea::setMessage(MessageType type, const QString& message)
{
    if (type == Status) {
        m_statusLabel->setText(message);
    } else {
        kDebug() << "ERROR message:" << message;
        //TODO handle error messages
    }
}

void StatusArea::onCallDurationTimerTimeout()
{
    m_callDuration = m_callDuration.addSecs(1);
    m_statusBar->showMessage(m_callDuration.toString(), 0);
}

void StatusArea::showAudioStatusIcon(bool show)
{
    if (show) {
        if (!m_audioStatusIcon) {
            QLabel *label = new QLabel;
            label->setPixmap(QIcon::fromTheme("audio-headset").pixmap(IconSize(KIconLoader::Small)));
            m_audioStatusIcon = label;
            m_statusBar->insertPermanentWidget(1, m_audioStatusIcon.data());
        } else {
            m_statusBar->insertPermanentWidget(1, m_audioStatusIcon.data());
            m_audioStatusIcon.data()->show();
        }
    } else {
        if (m_audioStatusIcon) {
            m_statusBar->removeWidget(m_audioStatusIcon.data());
        }
    }
}

void StatusArea::showVideoStatusIcon(bool show)
{
    if (show) {
        if (!m_videoStatusIcon) {
            QLabel *label = new QLabel;
            label->setPixmap(QIcon::fromTheme("camera-web").pixmap(IconSize(KIconLoader::Small)));
            m_videoStatusIcon = label;
            m_statusBar->insertPermanentWidget(1, m_videoStatusIcon.data());
        } else {
            m_statusBar->insertPermanentWidget(1, m_videoStatusIcon.data());
            m_videoStatusIcon.data()->show();
        }
    } else {
        if (m_videoStatusIcon) {
            m_statusBar->removeWidget(m_videoStatusIcon.data());
        }
    }
}
