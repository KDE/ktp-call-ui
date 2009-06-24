/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "kcallhandlersettingsdialog.h"
#include "ui_callwindowsettingspage.h"

class CallWindowSettingsPage : public QWidget, private Ui::CallWindowSettingsPage
{
public:
    CallWindowSettingsPage(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

KCallHandlerSettingsDialog::KCallHandlerSettingsDialog(QWidget *parent, KConfigSkeleton *config)
    : KConfigDialog(parent, "kcallhandlersettings", config)
{
    CallWindowSettingsPage *cwsp = new CallWindowSettingsPage(this);
    addPage(cwsp, i18n("Call Window"));
}

//static
bool KCallHandlerSettingsDialog::showDialog()
{
    return KConfigDialog::showDialog("kcallhandlersettings");
}

//static
void KCallHandlerSettingsDialog::addHandlerPagesToDialog(KConfigDialog *dialog,
                                                         KConfigSkeleton *handlerConfig)
{
    CallWindowSettingsPage *cwsp = new CallWindowSettingsPage(dialog);
    dialog->addPage(cwsp, handlerConfig, i18n("Call Window"));
}
