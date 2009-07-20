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
#include "tabledevicechooser.h"
#include "devicesmodel.h"
#include "ui_tabledevicechooser.h"
#include "../libqtgstreamer/qgstpipeline.h"
#include "../libqtgstreamer/qgstelementfactory.h"
#include <QtCore/QPointer>
#include <KLocalizedString>
#include <KDebug>
#include <KMessageBox>

namespace KGstDevices {

struct TableDeviceChooser::Private
{
    Private(DeviceManager *m, DeviceManager::DeviceType t, TableDeviceChooser *qq)
        : m_manager(m), m_type(t), q(qq) {}

    void onCurrentDeviceChanged(DeviceManager::DeviceType type);
    void onSelectionChanged(const QItemSelection & selected);
    void onModelReset();
    void testDevice();
    void toggleDetails();

    QPointer<DeviceManager> m_manager;
    DevicesModel *m_model;
    DeviceManager::DeviceType m_type;

    Ui::TableDeviceChooser m_ui;
    bool m_showingDetails;
    bool m_testing;
    QtGstreamer::QGstPipelinePtr m_testPipeline;

    TableDeviceChooser *const q;
};

TableDeviceChooser::TableDeviceChooser(DeviceManager *manager,
                                       DeviceManager::DeviceType type,
                                       QWidget *parent)
    : QWidget(parent), d(new Private(manager, type, this))
{
    d->m_ui.setupUi(this);

    d->m_model = new DevicesModel(this);
    d->m_model->populate(d->m_manager, d->m_type);
    d->m_ui.treeView->setModel(d->m_model);
    connect(d->m_model, SIGNAL(modelReset()), SLOT(onModelReset()));

    if ( d->m_manager->availableDevices(d->m_type).size() > 0 ) {
        d->onCurrentDeviceChanged(d->m_type);
    }
    connect(d->m_manager, SIGNAL(currentDeviceChanged(DeviceManager::DeviceType)),
            SLOT(onCurrentDeviceChanged(DeviceManager::DeviceType)));

    d->m_testing = false;
    d->m_ui.testButton->setText(i18nc("test the device", "Test"));
    d->m_ui.testButton->setIcon(KIcon("media-playback-start"));
    connect(d->m_ui.testButton, SIGNAL(clicked()), SLOT(testDevice()));

    d->m_showingDetails = true; //we are currently showing details...
    d->toggleDetails(); //...so, let's hide them
    connect(d->m_ui.detailsButton, SIGNAL(clicked()), SLOT(toggleDetails()));

    d->m_ui.addCustomDeviceButton->setText(i18n("Add custom device..."));
    d->m_ui.addCustomDeviceButton->setEnabled(false); //TODO implement this button
}

TableDeviceChooser::~TableDeviceChooser()
{
    delete d;
}

void TableDeviceChooser::Private::onCurrentDeviceChanged(DeviceManager::DeviceType type)
{
    if ( type != m_type ) {
        return; //we are only interested in one type
    }

    Device device = m_manager->currentDeviceForType(m_type);
    int row = m_manager->availableDevices(m_type).indexOf(device);
    QModelIndex left = m_model->index(row, 0);
    QModelIndex right = m_model->index(row, m_ui.treeView->isColumnHidden(3) ? 0 : 3);

    //disconnect to avoid infinite recursion when this function is called from onSelectionChanged
    disconnect(m_ui.treeView->selectionModel(),
               SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
               q, SLOT(onSelectionChanged(QItemSelection)));

    m_ui.treeView->selectionModel()->select(QItemSelection(left, right),
                                             QItemSelectionModel::ClearAndSelect);

    connect(m_ui.treeView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            q, SLOT(onSelectionChanged(QItemSelection)));
}

//this function will only be called when the user has selected a new row in the treeView.
void TableDeviceChooser::Private::onSelectionChanged(const QItemSelection & selected)
{
    if ( selected.isEmpty() ) {
        //if there is no selection, something is wrong. So, select the first available device.
        //normally this condition should never appear.
        if ( m_manager->availableDevices(m_type).size() > 0 ) {
            kDebug() << "No selection. Selecting the first device.";
            m_manager->setCurrentDeviceForType(m_type, m_manager->availableDevices(m_type)[0]);
        }
    } else {
        int row = selected.indexes()[0].row();
        m_manager->setCurrentDeviceForType(m_type, m_manager->availableDevices(m_type)[row]);
    }
}

void TableDeviceChooser::Private::onModelReset()
{
    if ( m_manager && m_manager->availableDevices(m_type).size() > 0 ) {
        //if there are devices for this type, the manager should already have one selected.
        //here we just update the treeview selection to reflect what the manager has selected.
        onCurrentDeviceChanged(m_type);
    }
    //if the manager has no available devices, there is nothing to do. No devices, no selection.
}

void TableDeviceChooser::Private::testDevice()
{
    using namespace QtGstreamer;
    if ( !m_testing ) {
        m_testPipeline = QGstPipeline::newPipeline();

        if ( m_type == DeviceManager::AudioOutput ) {
            QGstElementPtr audioTestSrc = QGstElementFactory::make("audiotestsrc");
            QGstElementPtr audioConvert = QGstElementFactory::make("audioconvert");
            QGstElementPtr audioResample = QGstElementFactory::make("audioresample");
            if ( !audioTestSrc || !audioConvert || !audioResample ) {
                KMessageBox::sorry(q, i18n("Some gstreamer elements could not be created. "
                                              "Please check your gstreamer installation."));
                m_testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            QGstElementPtr outputElement = m_manager->newAudioOutputElement();
            if ( !outputElement ) {
                KMessageBox::sorry(q, i18n("The gstreamer element for the selected device could "
                                        "not be created. Please check your gstreamer installation."));
                m_testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            } else if ( !outputElement->setState(QGstElement::Ready) ) {
                KMessageBox::sorry(q, i18n("The selected device could not be initialized. "
                                              "Please select another device."));
                m_testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            m_testPipeline->add(audioTestSrc);
            m_testPipeline->add(audioConvert);
            m_testPipeline->add(audioResample);
            m_testPipeline->add(outputElement);
            QGstElement::link(audioTestSrc, audioConvert, audioResample, outputElement);
        } else if ( m_type == DeviceManager::AudioInput ) {
            QGstElementPtr inputElement = m_manager->newAudioInputElement();
            if ( !inputElement ) {
                KMessageBox::sorry(q, i18n("The gstreamer element for the selected device could "
                                        "not be created. Please check your gstreamer installation."));
                m_testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            } else if ( !inputElement->setState(QGstElement::Ready) ) {
                KMessageBox::sorry(q, i18n("The selected device could not be initialized. "
                                              "Please select another device."));
                m_testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            QGstElementPtr audioConvert = QGstElementFactory::make("audioconvert");
            QGstElementPtr audioResample = QGstElementFactory::make("audioresample");
            if ( !audioConvert || !audioResample ) {
                KMessageBox::sorry(q, i18n("Some gstreamer elements could not be created. "
                                              "Please check your gstreamer installation."));
                m_testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            QGstElementPtr outputElement = m_manager->newAudioOutputElement();
            if ( !outputElement ) {
                KMessageBox::sorry(q, i18n("The gstreamer element for the audio output device "
                                              "could not be created. Please check your gstreamer "
                                              "installation"));
                m_testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            } else if ( !outputElement->setState(QGstElement::Ready) ) {
                KMessageBox::sorry(q, i18n("The audio output device could not be initialized."));
                m_testPipeline = QGstPipelinePtr(); //delete the pipeline, no reason to keep it.
                return;
            }

            m_testPipeline->add(inputElement);
            m_testPipeline->add(audioConvert);
            m_testPipeline->add(audioResample);
            m_testPipeline->add(outputElement);
            QGstElement::link(inputElement, audioConvert, audioResample, outputElement);
        }
        m_testPipeline->setState(QGstElement::Playing);
        m_testing = true;
        m_ui.testButton->setText(i18nc("stop audio test", "Stop test"));
        m_ui.testButton->setIcon(KIcon("media-playback-stop"));
    } else {
        m_testPipeline->setState(QGstElement::Null);
        m_testPipeline = QGstPipelinePtr(); //delete the pipeline
        m_testing = false;
        m_ui.testButton->setText(i18nc("test the device", "Test"));
        m_ui.testButton->setIcon(KIcon("media-playback-start"));
    }
}

void TableDeviceChooser::Private::toggleDetails()
{
    if ( m_showingDetails ) {
        m_ui.detailsButton->setText(i18n("Show details"));
        m_ui.treeView->hideColumn(1);
        m_ui.treeView->hideColumn(2);
        m_ui.treeView->hideColumn(3);
    } else {
        m_ui.detailsButton->setText(i18n("Hide details"));
        m_ui.treeView->showColumn(1);
        m_ui.treeView->showColumn(2);
        m_ui.treeView->showColumn(3);
    }
    m_ui.treeView->header()->resizeSections(QHeaderView::ResizeToContents);
    m_showingDetails = !m_showingDetails;
}

}

#include "tabledevicechooser.moc"
