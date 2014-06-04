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

Item{
  
  id: tooltip
  property alias text: label.text
  property bool containsMouse: parent.containsMouse
  visible: false
  
  anchors.horizontalCenter: parent.horizontalCenter
  anchors.top: parent.bottom
  
  width: label.width
  height: label.height
  
  Text{
    id: label
    font.pointSize: 7
    color: "white"
    text: "This is a tooltip"
  }
    Timer {
        id:showTimer
        interval: 1500
        running: (tooltip.containsMouse && !tooltip.visible)
        onTriggered: tooltip.visible=true
    }
    Timer {
        id:hideTimer
        interval: 100
        running: (!tooltip.containsMouse &&  tooltip.visible)
        onTriggered: tooltip.visible=false
    }  
}