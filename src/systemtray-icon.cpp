/*
    Copyright 2014  Ekaitz Zárraga <ekaitzzarraga@gmail.com>

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

#include "systemtray-icon.h"

SystemTrayIcon::SystemTrayIcon(QObject *parent)
: KStatusNotifierItem(parent)
{
    setIconByName("call-start");
    setCategory(KStatusNotifierItem::SystemServices);
    setStatus(KStatusNotifierItem::Passive);
    setStandardActionsEnabled(false);
    activateNextTime=true;
}

void SystemTrayIcon::show()
{
    if(activateNextTime) {
        setStatus(KStatusNotifierItem::Active);
        showMessage(contextMenu()->title(), i18n("The video call window has been hidden, but the call will remain active. Click to restore."), "call-start", 3000);
    }else{
        activateNextTime=true;
    }
}

void SystemTrayIcon::setActivateNext(bool activateNext)
{
    activateNextTime=activateNext;
}
