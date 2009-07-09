#include <QtGui>
#include <gst/gst.h>
#include "../../phonon/videowidget.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    gst_init(&argc, &argv);

    GstElement *pipeline = gst_pipeline_new(NULL);
    GstElement *videosrc = gst_element_factory_make("videotestsrc", NULL);
    VideoWidget *videoWidget = new VideoWidget;
    gst_bin_add_many(GST_BIN(pipeline), videosrc, videoWidget->videoElement(), NULL);
    gst_element_link(videosrc, videoWidget->videoElement());
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    videoWidget->show();
    return a.exec();
}
