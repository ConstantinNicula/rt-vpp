#include "pipeline.h"
#include "cam_utils.h"
#include "glib-object.h"
#include "gst/gstelement.h"
#include "gst/gstelementfactory.h"
#include "gst/gstpipeline.h"
#include "gst/gstutils.h"
#include "gst/gstvalue.h"
#include "log_utils.h"

static int create_decoding_stage(PipelineHandle *handle, CamParams *cam_params);
static int create_processing_stage(PipelineHandle *handle);
static int create_encoding_stage(PipelineHandle *handle);

static GstElement* create_caps_filter(const char* type, const char* format, int width, int height, int fr_num, int fr_denom); 
static void debug_print_caps(GstElement* elem, const char* pad);

#define RET_OK   0
#define RET_ERR -1 

/* Yeah I know macros are evil but I got tired of typing the same thing over and over again...*/
#define CHECK(expr, msg) do {\
    if (!(expr)) {\
        ERROR(msg);\
        return RET_ERR;\
    }\
} while(0)

int create_pipeline(CamParams *cam_params, PipelineHandle *handle) {
    int ret = 0;
    /* Create the empty pipeline */
    handle->pipeline = gst_pipeline_new("my-test-pipeline");
    CHECK(handle->pipeline != NULL, "Failed to create pipeline \n");

    /* Create stages */    
    ret = create_decoding_stage(handle, cam_params);
    CHECK(ret == 0, "Failed to create decoding stage of pipeline\n"); 

    ret = create_processing_stage(handle);
    CHECK(ret == 0, "Failed to create processing stage of pipeline");

    ret = create_encoding_stage(handle);
    CHECK(ret == 0, "Failed to create encoding stage of pipeline\n");

    return RET_OK;
}

static int create_decoding_stage(PipelineHandle* handle, CamParams* cam_params) {
    /* Create v4l2 source element */
    handle->dec.cam_source = gst_element_factory_make("v4l2src", "camera-source"); 
    CHECK(handle->dec.cam_source != NULL, "Failed to allocate v4l2src element");
    g_object_set(G_OBJECT(handle->dec.cam_source), 
                "device", cam_params->dev_path, NULL);
    DEBUG_PRINT_FMT("%s", cam_params->dev_path);

    // TO DO: SELECT ON PIXEL TYPE 
    /* Create capsfilter for source element */
    handle->dec.cam_caps_filter = create_caps_filter("video/x-raw", NULL, 
                    cam_params->width, cam_params->height, 
                    cam_params->fr_num, cam_params->fr_denom);
    CHECK(handle->dec.cam_caps_filter != NULL, "Failed to allocate camera caps filter\n");
    
    /* Add front end elements to pipeline */ 
    gst_bin_add_many(GST_BIN(handle->pipeline), 
                    handle->dec.cam_source, 
                    handle->dec.cam_caps_filter, 
                    NULL);

    debug_print_caps(handle->dec.cam_source, "src");
    debug_print_caps(handle->dec.cam_caps_filter, "sink");

    /* Link all elements */
    gboolean res = gst_element_link(handle->dec.cam_source, handle->dec.cam_caps_filter );
    DEBUG_PRINT_EXPR(res);
    CHECK(res == TRUE, "Failed to link elements\n");
        
    return RET_OK;
}

static int create_processing_stage(PipelineHandle *handle) {
    return RET_OK;
}

static int create_encoding_stage(PipelineHandle *handle) {
    return RET_OK;
}

static void debug_print_caps(GstElement* elem, const char* pad) {
    GstCaps *caps = gst_pad_query_caps(gst_element_get_static_pad(elem, pad), NULL);
    DEBUG_PRINT_FMT("%s caps: %s\n", pad, gst_caps_to_string(caps));
}

static GstElement* create_caps_filter(const char* type, const char* format, int width, int height, int fr_num, int fr_denom) {
    GstElement *caps_filter;
    GstCaps *caps;

    caps_filter = gst_element_factory_make("capsfilter", "");
    if (!caps_filter) {
        ERROR("Failed to create capsfilter element\n");
        return NULL;
    }

    /* Create a caps structure */
    if (!format) {
        caps = gst_caps_new_simple(type, 
                                    "width", G_TYPE_INT, width, 
                                    "height", G_TYPE_INT, height,
                                    "framerate", GST_TYPE_FRACTION, fr_denom, fr_num,
                                    NULL); 
    } else {
        caps = gst_caps_new_simple(type, 
                                    "format", G_TYPE_STRING, format,
                                    "width", G_TYPE_INT, width, 
                                    "height", G_TYPE_INT, height,
                                    "framerate", GST_TYPE_FRACTION, fr_denom, fr_num,
                                    NULL); 
    }
    /* Set the caps attribute on the capsfilter element*/
    g_object_set(G_OBJECT(caps_filter), "caps", caps, NULL);

    return caps_filter;
}
