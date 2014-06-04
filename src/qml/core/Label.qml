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

Column{
  id: label
  anchors.centerIn: parent
  property alias text: name.text
  property alias image: avatar.source
  spacing: 30
  
  Image{
    id: avatar
    height: 90
    width: 90
    source: ""
    fillMode: Image.PreserveAspectFit
  }
  Item{
    height: 30
    width: 90
    Text{
      id: name
      text: ""
      color: "white"
      anchors.centerIn: parent
    }
  }

}