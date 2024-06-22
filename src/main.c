#include "cam_utils.h"
#include "pipeline.h"

#include "gst/gstclock.h"
#include "gst/gstelement.h"
#include "gst/gstmessage.h"
#include "log_utils.h"
#include <gst/gst.h>

int test_pipeline(int argc, char** argv) {
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    PipelineHandle handle;
    CamParams cam_params  = {0};

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Read camera parameters */
    const char* dev = "/dev/video0";
    read_cam_params(dev, &cam_params);
 
    /* Create the elements */
    create_pipeline(&cam_params, &handle);
    GstElement* conv = gst_element_factory_make("videoconvert", "conv");
    GstElement* sink = gst_element_factory_make("autovideosink", "sink");
   
    if (!conv || !sink) {
        ERROR("Not all elements could be created. \n");
        return -1;
    }

    /* Build the pipeline */ 
    gst_bin_add_many(GST_BIN(handle.pipeline), conv, sink, NULL);
    if (gst_element_link_many(handle.dec.cam_caps_filter, conv, sink, NULL) != TRUE) {
        ERROR("Elements could not be linked");
        gst_object_unref(handle.pipeline);
        return -1;
    }

   
    /* Start playing */
    ret = gst_element_set_state(handle.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ERROR("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(handle.pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus(handle.pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR| GST_MESSAGE_EOS);

    /* Parse message */ 
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: 
                gst_message_parse_error(msg, &err, &debug_info);
                ERROR_FMT("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                ERROR_FMT("Debugging information: %s\n", debug_info ? debug_info: "none");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS: 
                DEBUG_PRINT("End-Of-Stream reached.\n");
                break;
            default: 
                /* We should no reach this point because we only asked for ERRORs and EOS */
                DEBUG_PRINT("Unexpected message received. \n");
                break;
        }
        gst_message_unref(msg);
    }

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(handle.pipeline, GST_STATE_NULL);
    gst_object_unref(handle.pipeline);
    return 0;
}  

int main(int argc, char** argv) {
   return test_pipeline(argc, argv);
}
