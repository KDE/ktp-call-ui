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

import QtQuick 2.0

Item {
    id: root
    width: toolsWidth
    height: toolsHeight

    signal hangup()
    signal hold()

    property int toolsWidth: menu.width + 20
    property int toolsHeight: menu.height + 30

    function setHoldEnabled(enable) {
        hold.setEnabled(enable);
    }

    function changeHoldIcon(icon) {
        if (icon == "start") {
            hold.iconSource= "image://icon/media-playback-start.png"
        } else {
            hold.iconSource= "image://icon/media-playback-pause.png"
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: "black"
        opacity: 0.7
        border.width: 1
        border.color: "dimgrey"
    }

    MouseArea {
        id: mouse
        hoverEnabled: true
        anchors.fill: parent

        onExited: {
            showTimer.restart();
        }

        onEntered: {
            root.state = "visible"
        }

        Timer {
            id:showTimer
            interval: 3000
            running: !parent.containsMouse

            onTriggered: {
                root.state= "hidden";
            }
        }
    }

    Row {
        id: menu
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            topMargin: 15
        }

        spacing: 10

        Button {
            id: hangup
            iconSource: "image://icon/call-stop.png"
            enabled: true

            onButtonClick: {
                root.hangup();
            }

            Tooltip {
                text: i18n("Hangup")
            }
        }

        Button {
            id: hold
            iconSource: "image://icon/media-playback-pause.png"
            enabled: false

            onButtonClick: {
                root.hold();
            }

            Tooltip {
                text: i18n("Hold")
            }
        }

        Separator {}

        ToggleButton {
            id: sound
            iconSource: "image://icon/audio-volume-medium.png"
            enabled: muteAction.enabled
            checked: muteAction.checked

            onButtonClick: {
                muteAction.toggle();
            }

            Tooltip {
                text: i18n("Mute")
            }
        }

        ToggleButton {
            id: showMyVideo
            iconSource: "image://icon/camera-web.png"
            enabled: showMyVideoAction.enabled
            checked: showMyVideoAction.checked

            onButtonClick: {
                showMyVideoAction.toggle();
            }

            Tooltip {
                text: i18n("Show My Video")
            }
        }

        Separator {}

        ToggleButton {
            id: showDialpad
            iconSource: "image://icon/phone.png"
            enabled: showDtmfAction.enabled
            checked: showDtmfAction.checked

            onButtonClick: {
                showDtmfAction.toggle();
            }

            Tooltip {
                text: i18n("Show Dialpad")
            }
        }
    }

    states: [
        State {
            name: "visible"
            PropertyChanges {
                target: root
                height: 60
            }
        },
        State {
            name: "hidden"
            PropertyChanges {
                target: root
                height:10
            }
        }
    ]

    transitions: [
        Transition {
            from: "visible"
            to: "hidden"

            PropertyAnimation {
                target: root
                properties: "height"
                easing.type: Easing.InOutQuad
                duration: 300
            }
        },
        Transition {
            from: "hidden"
            to: "visible"

            PropertyAnimation {
                target: root
                properties: "height"
                easing.type: Easing.InOutQuad
            }
        }
    ]
}
