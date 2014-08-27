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

Rectangle {
    id: root
    height: 50
    width: 55

    signal buttonClick()
    signal buttonPressed()
    signal buttonReleased()

    property alias number: number.text

    smooth: true
    radius: 10

    border.width: 1
    border.color: "white"

    gradient: Gradient {
        GradientStop {position: 0.0; color: "grey" }
        GradientStop {position: 0.33; color: "black" }
        GradientStop {position: 1.0; color: "dimgrey" }
    }

    Text {
        id: number
        anchors.centerIn: parent

        text: ""
        font.pixelSize: 20
        color: "white"
    }

    MouseArea {
        id: mouse
        anchors.fill: parent

        hoverEnabled: true
        onClicked: {
            root.buttonClick();
        }

        onPressed: {
            number.color = "white";
            border.color = "grey";
            root.scale = 0.92;
            root.buttonPressed();
        }

        onReleased: {
            number.color = "white";
            border.color = "white";
            root.scale = 1;
            root.buttonReleased();
        }

        onEntered: {
            root.opacity = 0.8;
        }

        onExited: {
            root.opacity = 1;
        }
    }
}
