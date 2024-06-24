#include "cam_utils.h"
#include "gst/gstdebugutils.h"
#include "pipeline.h"

#include "log_utils.h"
#include "shader_utils.h"
#include <gst/gst.h>

int test_pipeline(int argc, char** argv) {
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    PipelineHandle handle = {0};
    PipelineConfig pipeline_config = {0};
    CamParams cam_params  = {0};

     /* Initialize GStreamer */
    gst_init(&argc, &argv);
   
    init_shader_store();
    if (add_shaders_to_store("./shaders") == RET_OK) 
        DEBUG_PRINT("Loaded all shaders\n");
    
    /* Read camera parameters */
    const char* dev = "/dev/video0";
    if (read_cam_params(dev, &cam_params) != RET_OK) return RET_ERR;
 
    /* Create the elements */
    get_default_pipeline_config(&pipeline_config);
    pipeline_config.shader_pipeline = "horizontal_flip ! ripple_effect ! invert_color";
    pipeline_config.out_height = 900;
    pipeline_config.out_width = 900;
    pipeline_config.dev_sink = "/dev/video2";
    if (create_pipeline(&cam_params, &pipeline_config, &handle) != RET_OK) return RET_ERR; 
   
    /* Start playing */
    ret = gst_element_set_state(handle.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ERROR("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(handle.pipeline);
        return -1;
    }
    gst_debug_bin_to_dot_file(GST_BIN(handle.pipeline), GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS, "testfile.dot");
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
    cleanup_shader_store();
    return RET_OK;
}  

int main(int argc, char** argv) {
   return test_pipeline(argc, argv);
}
