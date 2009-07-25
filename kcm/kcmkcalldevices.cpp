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
#include "kcmkcalldevices.h"
#include "../libqtgstreamer/qgstglobal.h"
#include "../libkgstdevices/devicemanager.h"
#include "../libkgstdevices/tabledevicechooser.h"
#include <QtGui/QVBoxLayout>
#include <KPluginFactory>
#include <KTabWidget>
#include <KLocalizedString>
#include <KIcon>
#include <KConfigGroup>

K_PLUGIN_FACTORY(KcmKcallDevicesFactory, registerPlugin<KcmKcallDevices>(); )
K_EXPORT_PLUGIN(KcmKcallDevicesFactory("kcmkcalldevices", "kcall"))

using namespace KGstDevices;

struct KcmKcallDevices::Private
{
    DeviceManager *manager;
    TableDeviceChooser *audioInputChooser;
    TableDeviceChooser *audioOutputChooser;
    TableDeviceChooser *videoInputChooser;
    TableDeviceChooser *videoOutputChooser;
};

KcmKcallDevices::KcmKcallDevices(QWidget *parent, const QVariantList & args)
    : KCModule(KcmKcallDevicesFactory::componentData(), parent, args), d(new Private)
{
    setButtons(Default | Apply);

    QtGstreamer::qGstInit();
    d->manager = new DeviceManager(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    KTabWidget *tabWidget = new KTabWidget(this);
    layout->addWidget(tabWidget);

    d->audioInputChooser = new TableDeviceChooser(d->manager, DeviceManager::AudioInput);
    d->audioOutputChooser = new TableDeviceChooser(d->manager, DeviceManager::AudioOutput);
    d->videoInputChooser = new TableDeviceChooser(d->manager, DeviceManager::VideoInput);
    d->videoOutputChooser = new TableDeviceChooser(d->manager, DeviceManager::VideoOutput);
    tabWidget->addTab(d->audioInputChooser, KIcon("audio-input-microphone"), i18n("Audio input device"));
    tabWidget->addTab(d->audioOutputChooser, KIcon("audio-card"), i18n("Audio output device"));
    tabWidget->addTab(d->videoInputChooser, KIcon("camera-web"), i18n("Video input device"));
    tabWidget->addTab(d->videoOutputChooser, KIcon("video-display"), i18n("Video output driver"));
}

KcmKcallDevices::~KcmKcallDevices()
{
    delete d;
}

void KcmKcallDevices::load()
{
    disconnect(d->manager, SIGNAL(currentDeviceChanged(DeviceManager::DeviceType)), this, SLOT(changed()));
    d->manager->loadConfig(KSharedConfig::openConfig("kcall_handlerrc")->group("Devices"));
    connect(d->manager, SIGNAL(currentDeviceChanged(DeviceManager::DeviceType)), this, SLOT(changed()));
}

void KcmKcallDevices::save()
{
    d->manager->saveConfig(KSharedConfig::openConfig("kcall_handlerrc")->group("Devices"));
}

void KcmKcallDevices::defaults()
{
    d->manager->loadDefaults();
}

#include "kcmkcalldevices.moc"
