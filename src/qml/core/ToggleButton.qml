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

Item {
    id: root
    height: 30
    width: 30

    signal buttonClick(bool toggled)

    property bool enabled: false
    property alias containsMouse: area.containsMouse
    property bool checked: false

    property alias iconSource: icon.source

    Rectangle {
        id: container
        anchors.fill: parent
        color: "transparent"
        opacity: root.enabled ? 1 : 0.5

        Image {
            id: icon
            anchors.centerIn: parent
            source: ""
            smooth: true
        }

        Image {
            id: activated
            width: 15
            height: 15

            anchors {
                right: parent.right
                bottom: parent.bottom
            }
            source: "image://icon/application-exit.png"
            visible: false
            state: enabled && checked ? "" : "unchecked"

            states: [
                State {
                    name: ""
                    PropertyChanges {target: activated; visible: false}
                    },
                State {
                    name: "unchecked"
                    PropertyChanges {target: activated; visible: true}
                }
            ]
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                if (root.enabled) {
                    if (activated.state == "") {
                        root.buttonClick(false);
                        root.activate(false);
                    } else {
                        root.buttonClick(true)
                        root.activate(true)
                    }
                }
            }

            onPressed: {
                if (root.enabled) {
                    icon.scale = 0.9;
                }
            }

            onReleased: {
                icon.scale = 1.0;
            }

            onEntered: {
                if (root.enabled) {
                    container.opacity = 0.7;
                }
            }

            onExited: {
                if (root.enabled) {
                    container.opacity = 1
                }
            }
        }
    }
}
