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

#ifndef SYSTEMTRAYICON
#define SYSTEMTRAYICON

#include <KStatusNotifierItem>
#include <KMenu>
#include <KLocalizedString>

class SystemTrayIcon : public KStatusNotifierItem
{

    Q_OBJECT

public:
    SystemTrayIcon(QObject *parent = 0);
    void show();
    void setActivateNext(bool activateNext);

private:
    bool activateNextTime;
};

#endif //SYSTEMTRAYICON
