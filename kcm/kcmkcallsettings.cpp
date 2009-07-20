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
#include "ui_callwindowsettingspage.h"
#include "kcallhandlersettings.h"
#include <QtGui/QVBoxLayout>
#include <KPluginFactory>
#include <KCModule>

class CallWindowSettingsPage : public QWidget, private Ui::CallWindowSettingsPage
{
public:
    CallWindowSettingsPage(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class KcmKCallSettings : public KCModule
{
public:
    KcmKCallSettings(QWidget *parent, const QVariantList & args);
};


K_PLUGIN_FACTORY(KcmKCallSettingsFactory, registerPlugin<KcmKCallSettings>(); )
K_EXPORT_PLUGIN(KcmKCallSettingsFactory("kcmkcallsettings", "kcall"))

KcmKCallSettings::KcmKCallSettings(QWidget *parent, const QVariantList & args)
    : KCModule(KcmKCallSettingsFactory::componentData(), parent, args)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    CallWindowSettingsPage *cwsp = new CallWindowSettingsPage(this);
    layout->addWidget(cwsp);

    addConfig(KCallHandlerSettings::self(), cwsp);
}
