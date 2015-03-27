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

import QtQuick 1.1
import QtGStreamer 1.0
import "core"

Rectangle {
    id: root
    height: 450
    width: 750
    color: "black"

    //SIGNALS to outside
    signal showDialpadClicked(bool checked)
    signal showDialpadChangeState(bool checked)
    signal hangupClicked()
    signal holdClicked()
    signal muteClicked(bool checked)
    signal exitFullScreen()

    focus: true
    Keys.enabled: true
    Keys.onEscapePressed: root.exitFullScreen();

    function changeHoldIcon(icon) {
       toolbar.changeHoldIcon(icon);
    }

    function setLabel(name, imageUrl) {
       label.text = name;
       label.image = imageUrl;
    }

    function showVideo(show) {
      videoWidget.visible = show;
      label.visible = !show;
    }

    function setHoldEnabled(enable) {
        toolbar.setHoldEnabled(enable);
    }

    Rectangle {
        id: receivingVideo
        x: 70
        y: 10

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: parent.bottom

            topMargin: 10
            leftMargin: 70
            rightMargin: 70
            bottomMargin: 20
        }

        border.width: 2
        color: "transparent"

        Label {
            id: label
            text: i18n("Calling")
            image: ""
            visible: true
        }

        VideoItem {
            id: videoWidget
            anchors.fill: parent

            surface: videoSurface
            visible: false
        }
    }

    Rectangle {
        id: sendingVideo
        width: 200
        height: 150

        anchors {
            right: parent.right
            bottom: parent.bottom
            rightMargin: 20
            bottomMargin: 70
        }

        border.width: 2
        border.color: "dimgray"
        visible: showMyVideoAction.checked

        VideoItem {
            id: videoPreviewWidget
            anchors.fill: parent
            surface: videoPreviewSurface
        }
    }

    Toolbar {
        id: toolbar
        width: parent.width

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }

        onHangup: {
            root.hangupClicked();
        }

        onHold: {
            root.holdClicked();
        }
    }
}
