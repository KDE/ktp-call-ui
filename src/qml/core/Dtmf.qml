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

Rectangle{

  id: root

  height: all.height
  width:numpad.width
  color: "black"

  signal click(string number)
  signal press(string number)
  signal release(string number)

  onClick: dialed.text=dialed.text+number

  Column{
    id:all
    spacing: 10

    Rectangle{
      width: numpad.width
      height: 30
      color: "transparent"

	Rectangle{
	  radius: 6
	  smooth: true
	  anchors.top: parent.top
	  anchors.topMargin: 5
	  anchors.right: parent.right
	  anchors.rightMargin: 5
	  anchors.left: parent.left
	  anchors.leftMargin: 5
	  anchors.bottom: parent.bottom



	  border.width: 3
	  border.color: "white"
	  color: "black"

	TextInput{
	  id: dialed
	  color: "white"
	  height: 30
	  anchors.left: parent.left
	  anchors.leftMargin: 20
	  anchors.top: parent.top
	  anchors.topMargin: 7

	  anchors.right: parent.right
	  anchors.rightMargin: 20

      readOnly: true

	}
      }
    }
  Grid{
    id: numpad
    columns: 3
    spacing: 1
    DtmfButton{number:"1"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"2"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"3"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"4"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"5"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"6"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"7"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"8"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"9"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"*"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"0"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
    DtmfButton{number:"#"; onButtonClick:root.click(number); onButtonPressed: root.press(number); onButtonReleased: root.release(number)}
  }
  }

}
