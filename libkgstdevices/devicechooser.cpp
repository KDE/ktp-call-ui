/*
    Copyright (C) 2009  George Kiagiadakis <kiagiadakis.george@gmail.com>

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
#include "devicechooser_p.h"
#include "../libqtgstreamer/qgstelementfactory.h"
#include <KLocalizedString>
#include <KDebug>
#include <KMessageBox>

namespace KGstDevices {

//static
DeviceChooser *DeviceChooser::newDeviceChooser(DeviceManager *manager,
                                               DeviceManager::DeviceType type,
                                               QWidget *parent)
{
    switch (type) {
    case DeviceManager::AudioInput:
    case DeviceManager::AudioOutput:
        return new AudioDeviceChooser(manager, type, parent);
    case DeviceManager::VideoInput:
    case DeviceManager::VideoOutput:
    default:
        Q_ASSERT(false);
    }
    return NULL; //warnings--
}

DeviceChooser::DeviceChooser(DeviceChooserPrivate *dd, QWidget *parent)
    : QWidget(parent), d_ptr(dd)
{
    Q_D(DeviceChooser);
    d->model = new DevicesModel(this);
}

DeviceChooser::~DeviceChooser()
{
    delete d_ptr;
}

Device DeviceChooser::selectedDevice() const
{
    Q_D(const DeviceChooser);
    return d->manager->currentDeviceForType(d->type);
}

void DeviceChooser::setSelectedDevice(const Device & device)
{
    Q_D(DeviceChooser);
    d->manager->setDeviceForType(d->type, device);
}


AudioDeviceChooser::AudioDeviceChooser(DeviceManager *manager,
                                       DeviceManager::DeviceType type,
                                       QWidget *parent)
    : DeviceChooser(new AudioDeviceChooserPrivate(manager, type), parent)
{
    Q_ASSERT(type == DeviceManager::AudioInput || type == DeviceManager::AudioOutput);
    Q_D(AudioDeviceChooser);
    d->ui.setupUi(this);

    d->model->populate(d->manager, d->type);
    d->ui.treeView->setModel(d->model);
    connect(d->model, SIGNAL(modelReset()), SLOT(onModelReset()));

    if ( d->manager->availableDevices(type).size() > 0 ) {
        setSelectedDevice(d->manager->currentDeviceForType(d->type));
    }

    d->testing = false;
    d->ui.testButton->setText(i18nc("test the device", "Test"));
    d->ui.testButton->setIcon(KIcon("media-playback-start"));
    connect(d->ui.testButton, SIGNAL(clicked()), SLOT(testDevice()));

    d->showingDetails = true; //we are currently showing details...
    toggleDetails(); //...so, let's hide them
    connect(d->ui.detailsButton, SIGNAL(clicked()), SLOT(toggleDetails()));

    d->ui.addCustomDeviceButton->setText(i18n("Add custom device..."));
    d->ui.addCustomDeviceButton->setEnabled(false); //TODO implement this button
}

AudioDeviceChooser::~AudioDeviceChooser()
{
}

void AudioDeviceChooser::setSelectedDevice(const Device & device)
{
    Q_D(AudioDeviceChooser);
    int row = d->manager->availableDevices(d->type).indexOf(device);
    QModelIndex left = d->model->index(row, 0);
    QModelIndex right = d->model->index(row, d->ui.treeView->isColumnHidden(3) ? 0 : 3);

    //disconnect to avoid infinite recursion when this function is called from onSelectionChanged
    disconnect(d->ui.treeView->selectionModel(),
               SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
               this, SLOT(onSelectionChanged(QItemSelection)));

    d->ui.treeView->selectionModel()->select(QItemSelection(left, right),
                                             QItemSelectionModel::ClearAndSelect);

    connect(d->ui.treeView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            SLOT(onSelectionChanged(QItemSelection)));

    DeviceChooser::setSelectedDevice(device);
}

//this function will only be called when the user has selected a new row in the treeView.
void AudioDeviceChooser::onSelectionChanged(const QItemSelection & selected)
{
    Q_D(AudioDeviceChooser);
    if ( selected.isEmpty() ) {
        //if there is no selection, something is wrong. So, select the first available device.
        //normally this condition should never appear.
        if ( d->manager->availableDevices(d->type).size() > 0 ) {
            kDebug() << "No selection. Selecting the first device.";
            setSelectedDevice(d->manager->availableDevices(d->type)[0]);
        }
    } else {
        int row = selected.indexes()[0].row();
        DeviceChooser::setSelectedDevice(d->manager->availableDevices(d->type)[row]);
    }
}

void AudioDeviceChooser::onModelReset()
{
    Q_D(AudioDeviceChooser);
    if ( d->manager && d->manager->availableDevices(d->type).size() > 0 ) {
        //if there are devices for this type, the manager should already have one selected.
        //here we just update the treeview selection to reflect what the manager has selected.
        setSelectedDevice(d->manager->currentDeviceForType(d->type));
    }
    //if the manager has no available devices, there is nothing to do. No devices, no selection.
}

void AudioDeviceChooser::testDevice()
{
    Q_D(AudioDeviceChooser);
    using namespace QtGstreamer;
    if ( !d->testing ) {
        d->testPipeline = QGstPipeline::newPipeline();

        if ( d->type == DeviceManager::AudioOutput ) {
            QGstElementPtr audioTestSrc = QGstElementFactory::make("audiotestsrc");
            QGstElementPtr audioConvert = QGstElementFactory::make("audioconvert");
            QGstElementPtr audioResample = QGstElementFactory::make("audioresample");
            if ( !audioTestSrc || !audioConvert || !audioResample ) {
                KMessageBox::sorry(this, i18n("Some gstreamer elements could not be created. "
                                              "Please check your gstreamer installation."));
                d->testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            QGstElementPtr outputElement = d->manager->newAudioOutputElement();
            if ( !outputElement ) {
                KMessageBox::sorry(this, i18n("The gstreamer element for the selected device could "
                                        "not be created. Please check your gstreamer installation."));
                d->testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            } else if ( !outputElement->setState(QGstElement::Ready) ) {
                KMessageBox::sorry(this, i18n("The selected device could not be initialized. "
                                              "Please select another device."));
                d->testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            d->testPipeline->add(audioTestSrc);
            d->testPipeline->add(audioConvert);
            d->testPipeline->add(audioResample);
            d->testPipeline->add(outputElement);
            QGstElement::link(audioTestSrc, audioConvert, audioResample, outputElement);
        } else if ( d->type == DeviceManager::AudioInput ) {
            QGstElementPtr inputElement = d->manager->newAudioInputElement();
            if ( !inputElement ) {
                KMessageBox::sorry(this, i18n("The gstreamer element for the selected device could "
                                        "not be created. Please check your gstreamer installation."));
                d->testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            } else if ( !inputElement->setState(QGstElement::Ready) ) {
                KMessageBox::sorry(this, i18n("The selected device could not be initialized. "
                                              "Please select another device."));
                d->testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            QGstElementPtr audioConvert = QGstElementFactory::make("audioconvert");
            QGstElementPtr audioResample = QGstElementFactory::make("audioresample");
            if ( !audioConvert || !audioResample ) {
                KMessageBox::sorry(this, i18n("Some gstreamer elements could not be created. "
                                              "Please check your gstreamer installation."));
                d->testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            QGstElementPtr outputElement = d->manager->newAudioOutputElement();
            if ( !outputElement ) {
                KMessageBox::sorry(this, i18n("The gstreamer element for the audio output device "
                                              "could not be created. Please check your gstreamer "
                                              "installation"));
                d->testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            } else if ( !outputElement->setState(QGstElement::Ready) ) {
                KMessageBox::sorry(this, i18n("The audio output device could not be initialized."));
                d->testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            d->testPipeline->add(inputElement);
            d->testPipeline->add(audioConvert);
            d->testPipeline->add(audioResample);
            d->testPipeline->add(outputElement);
            QGstElement::link(inputElement, audioConvert, audioResample, outputElement);
        }
        d->testPipeline->setState(QGstElement::Playing);
        d->testing = true;
        d->ui.testButton->setText(i18nc("stop audio test", "Stop test"));
        d->ui.testButton->setIcon(KIcon("media-playback-stop"));
    } else {
        d->testPipeline->setState(QGstElement::Null);
        d->testPipeline = QGstPipelinePtr(); //delete the pipeline
        d->testing = false;
        d->ui.testButton->setText(i18nc("test the device", "Test"));
        d->ui.testButton->setIcon(KIcon("media-playback-start"));
    }
}

void AudioDeviceChooser::toggleDetails()
{
    Q_D(AudioDeviceChooser);
    if ( d->showingDetails ) {
        d->ui.detailsButton->setText(i18n("Show details"));
        d->ui.treeView->hideColumn(1);
        d->ui.treeView->hideColumn(2);
        d->ui.treeView->hideColumn(3);
    } else {
        d->ui.detailsButton->setText(i18n("Hide details"));
        d->ui.treeView->showColumn(1);
        d->ui.treeView->showColumn(2);
        d->ui.treeView->showColumn(3);
    }
    d->ui.treeView->header()->resizeSections(QHeaderView::ResizeToContents);
    d->showingDetails = !d->showingDetails;
}

}

#include "devicechooser.moc"
#include "devicechooser_p.moc"
