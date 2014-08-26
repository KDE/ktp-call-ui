/*
 *  Copyright (C) 2014 Ekaitz Zárraga <ekaitz.zarraga@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QGraphicsObject>
#include <QGst/ElementFactory>
#include <QGst/Ui/GraphicsVideoSurface>
#include <QGst/Init>

#ifndef QMLINTERFACE_H
#define QMLINTERFACE_H


//! Works as interface between the QML GUI and CallWindow. \a Ekaitz.
/*!
 * Manages the exchange of signals and function calls between the CallWindow and QML and sets up and controls the video players.
 *
 * Signals:
 *
 * CallWindow -> QML:
 *
 * <em> soundChangeState(), showMyVideoChangeState(), showDialpadChangeState() </em>
 *
 * QML -> CallWindow:
 *
 * <em>    hangupClicked(), holdClicked(), muteClicked(), showMyVideoClicked(), showDialpadClicked(), exitFullScreen() </em>
 */

class CallWindow;

class QmlInterface: public QDeclarativeView {
    Q_OBJECT

public:
    QmlInterface(CallWindow* parent = 0);
    virtual ~QmlInterface();

    void setLabel(const QString name, const QString imageUrl);
    void setShowVideo(bool show);
    void setChangeHoldIcon(QString icon);

    QGst::ElementPtr getVideoSink();
    QGst::ElementPtr getVideoPreviewSink();

public Q_SLOTS:
    void setHoldEnabled(bool enable);

Q_SIGNALS:
    //to outside
    void hangupClicked();
    void holdClicked();

private:
    void setupSignals();

private:
    struct Private;
    Private *const d;
};

#endif //QMLINTERFACE_H
