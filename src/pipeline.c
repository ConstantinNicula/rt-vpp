#include "pipeline.h"
#include "glib-object.h"
#include "gst/gstelement.h"
#include "gst/gstvalue.h"
#include "log_utils.h"
#include "cam_utils.h"
#include "shader_utils.h"
#include <time.h>


static int create_decoding_stage(PipelineHandle *handle, CamParams *cam_params);
static int create_processing_stage(PipelineHandle *handle, CamParams* cam_params, PipelineConfig* pipeline_config);
static int create_encoding_stage(PipelineHandle *handle, PipelineConfig* pipeline_config);
static int create_output_stage(PipelineHandle *handle, PipelineConfig* pipeline_config);


static GstElement* create_caps_filter(const char* type, const char* name, const char* format, 
                                        int width, int height, int fr_num, int fr_denom);
static int create_shader_pipeline_from_string(PipelineHandle *handle, const char* shader_pipeline);
static GstElement* create_shader(const char* shader_name); 

//#define DEBUT_SHOW_CAPS
#ifdef DEBUG_SHOW_CAPS
static void debug_print_caps(GstElement* elem, const char* pad);
#endif

void get_default_pipeline_config(PipelineConfig *out_pipeline_config) {
    *out_pipeline_config = (PipelineConfig){
        .dev_src = "/dev/video0",
        .shader_pipeline = "vertical_flip ! invert_color",
        .shader_src_folder = "./shaders",
        .bitrate = 2000, 
        .out_height = -1, 
        .out_width = -1, 
        .dev_sink = NULL, 
    };
}

int create_pipeline(CamParams *cam_params, PipelineConfig *pipeline_config, PipelineHandle *handle) {
    int create_res = 0;
    gboolean link_res = FALSE;

    /* 1) Create the empty pipeline */
    handle->pipeline = gst_pipeline_new("processing-pipeline");
    CHECK(handle->pipeline != NULL, "Failed to create pipeline", RET_ERR);

    /* 2) Create stages */    
    create_res = create_decoding_stage(handle, cam_params);
    CHECK(create_res == 0, "Failed to create decoding stage of pipeline", RET_ERR); 

    create_res = create_processing_stage(handle, cam_params, pipeline_config);
    CHECK(create_res == 0, "Failed to create processing stage of pipeline", RET_ERR);

    create_res = create_encoding_stage(handle, pipeline_config);
    CHECK(create_res == 0, "Failed to create encoding stage of pipeline", RET_ERR);

    create_res = create_output_stage(handle, pipeline_config);
    CHECK(create_res == 0, "Failed to create output stage of pipeline", RET_ERR);

    /* 3) Link stages */
    link_res = gst_element_link(handle->dec.out_caps_filter, handle->proc.uploader);  
    CHECK(link_res == TRUE, "Failed to link decode and processing stages of the pipeline", RET_ERR);

    link_res = gst_element_link(handle->proc.out_caps_filter, handle->enc.converter);
    CHECK(link_res == TRUE, "Failed to link processing and encoding stages of the pipeline", RET_ERR);

    link_res = gst_element_link(handle->enc.out_caps_filter, handle->out.tee);
    CHECK(link_res == TRUE, "Failed to link encoding and output stages of the pipeline", RET_ERR);

    return RET_OK;
}

int play_pipeline(PipelineHandle *handle) {
    GstBus *bus = NULL;
    GstMessage *msg = NULL;
    GstStateChangeReturn ret;

    /* Start playing */
    ret = gst_element_set_state(handle->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ERROR("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(handle->pipeline);
        return RET_ERR;
    }

    /* DEBUG: output dot file describing pipeline */
    gst_debug_bin_to_dot_file(GST_BIN(handle->pipeline), GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS, "debug_pipeline_nodes.dot");

    /* Wait until error or EOS */
    bus = gst_element_get_bus(handle->pipeline);
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
    gst_element_set_state(handle->pipeline, GST_STATE_NULL);
    gst_object_unref(handle->pipeline);

    return RET_OK;
}

static int create_decoding_stage(PipelineHandle* handle, CamParams* cam_params) {
    /* 1) Create v4l2 source element */
    handle->dec.cam_source = gst_element_factory_make("v4l2src", "camera-source"); 
    CHECK(handle->dec.cam_source != NULL, "Failed to allocate v4l2src element", RET_ERR);
    g_object_set(G_OBJECT(handle->dec.cam_source), 
                "device", cam_params->dev_path, NULL);
    
    /* 2) Create capsfilter for source element & decoder */
    DEBUG_PRINT_FMT("GStreamer compatible source format %s\n", pixel_format_to_str(cam_params->pixelformat));
    switch (cam_params->pixelformat) {
        case PIX_FMT_RGB24:
        case PIX_FMT_BGR24:
        case PIX_FMT_I420:
        case PIX_FMT_YUY2:
            handle->dec.cam_caps_filter = create_caps_filter("video/x-raw", "camera-capsfilter", 
                            pixel_format_to_str(cam_params->pixelformat), 
                            cam_params->width, cam_params->height, 
                            cam_params->fr_num, cam_params->fr_denom);
            handle->dec.decoder = NULL;
            break;
        case PIX_FMT_MJPG:
            handle->dec.cam_caps_filter = create_caps_filter("image/jpeg", "camera-capsfilter", 
                            pixel_format_to_str(cam_params->pixelformat), 
                            cam_params->width, cam_params->height, 
                            cam_params->fr_num, cam_params->fr_denom);
            handle->dec.decoder = gst_element_factory_make("avdec_mjpeg", "camera-decoder");
            CHECK(handle->dec.decoder != NULL, "Failed to allocate camera decoder", RET_ERR);
            break;
        default:
            ERROR("Failed to create caps filter! Unsupported pixel format");
            return RET_ERR;
    }
    CHECK(handle->dec.cam_caps_filter != NULL, "Failed to allocate camera capsfilter", RET_ERR);

    /* 3) Create video converter */
    handle->dec.converter = gst_element_factory_make("videoconvert", "camera-convert");
    CHECK(handle->dec.converter != NULL, "Failed to allocate camera converter", RET_ERR);

    /* 4) Create output capsfilter */ 
    handle->dec.out_caps_filter = create_caps_filter("video/x-raw", "output-capsfilter",
                                "RGBA", 
                                cam_params->width, cam_params->height,
                                cam_params->fr_num, cam_params->fr_denom);
    CHECK(handle->dec.out_caps_filter != NULL, "Failed to allocate output camera capsfilter", RET_ERR);
    
    /* 5) Add front end elements to pipeline */ 
    gst_bin_add_many(GST_BIN(handle->pipeline), 
                     handle->dec.cam_source, 
                     handle->dec.cam_caps_filter,
                     handle->dec.converter, 
                     handle->dec.out_caps_filter,
                     handle->dec.decoder, /* Adding it last cause it might be NULL*/
                     NULL);
    
    /* 6) Link all elements */
    gboolean res = TRUE;
    if (handle->dec.decoder) {
        res = gst_element_link_many(handle->dec.cam_source,
                              handle->dec.cam_caps_filter, 
                              handle->dec.decoder, 
                              handle->dec.converter, 
                              handle->dec.out_caps_filter, NULL);
    } else {
        res = gst_element_link_many(handle->dec.cam_source,
                              handle->dec.cam_caps_filter, 
                              handle->dec.converter, 
                              handle->dec.out_caps_filter, NULL);
    }
    CHECK(res == TRUE, "Failed to link elements", RET_ERR);
        
    return RET_OK;
}

static int create_processing_stage(PipelineHandle *handle, CamParams* cam_params, PipelineConfig* pipeline_config) {
    int num_shaders = 0;
    /* 1) Create gluploader */
    handle->proc.uploader = gst_element_factory_make("glupload", "proc-upload");
    CHECK(handle->proc.uploader != NULL, "Failed to allocate glupload element", RET_ERR);

    /* 2) Create glshader instances */
    num_shaders = create_shader_pipeline_from_string(handle, pipeline_config->shader_pipeline);
    CHECK(num_shaders != RET_ERR, "Failed to create entire shader pipeline", RET_ERR);

    /* 3) Create gldownloader*/
    handle->proc.downloader = gst_element_factory_make("gldownload", "proc-download");
    CHECK(handle->proc.downloader != NULL, "Failed to allocate gldownlaod element", RET_ERR);

    /* 4) Create videoscaler*/
    handle->proc.scaler = gst_element_factory_make("videoscale", "proc-videoscale");
    CHECK(handle->proc.scaler != NULL, "Failed to allocate videoscale element", RET_ERR);
    g_object_set(G_OBJECT(handle->proc.scaler), "add-borders", 0, NULL);

    /* 5) Create out caps_filter */
    handle->proc.out_caps_filter = create_caps_filter("video/x-raw", "proc-out-capsfilter", "RGBA", 
                pipeline_config->out_width > 0 ? pipeline_config->out_width : cam_params->width, 
                pipeline_config->out_height > 0 ? pipeline_config->out_height : cam_params->height, 
                cam_params->fr_num, cam_params->fr_denom);

    /* 6) Add all elements */
    gst_bin_add(GST_BIN(handle->pipeline), handle->proc.uploader);
    for (int idx = 0; idx < num_shaders; idx++) {
        gst_bin_add(GST_BIN(handle->pipeline), handle->proc.shader_stages[idx]);
    }
    gst_bin_add_many(GST_BIN(handle->pipeline), handle->proc.downloader, 
                    handle->proc.scaler, handle->proc.out_caps_filter, NULL);
    
    /* 7) Link all elements*/
    gst_element_link(handle->proc.uploader, handle->proc.shader_stages[0]);
    for (int idx = 1; idx < num_shaders; idx++) {
        DEBUG_PRINT_FMT("Linking shaders %d %d\n", idx-1, idx);
        gst_element_link(handle->proc.shader_stages[idx-1], handle->proc.shader_stages[idx]);
    }
    gst_element_link_many(handle->proc.shader_stages[num_shaders-1], handle->proc.downloader, 
                        handle->proc.scaler, handle->proc.out_caps_filter, NULL);
#ifdef DEBUT_SHOW_CAPS
    debug_print_caps(handle->proc.scaler, "sink");
#endif
    return RET_OK;
}

static int create_encoding_stage(PipelineHandle *handle, PipelineConfig* pipeline_config) {
    /* 1) Create converter stage */
    handle->enc.converter = gst_element_factory_make("videoconvert", "enc-convert");
    CHECK(handle->enc.converter != NULL, "Failed to allocate videoconvert element", RET_ERR);

    /* 2) Create encoder stage */ 
    handle->enc.encoder = gst_element_factory_make("x264enc", "enc-h264");
    CHECK(handle->enc.encoder != NULL, "Failed to allocate x264enc element", RET_ERR);
    g_object_set(G_OBJECT(handle->enc.encoder), 
                "bitrate", pipeline_config->bitrate, 
                "tune", 4, // zerolatency mode 
                "speed-preset", 2, // superfast mode  
                NULL);
    
    /* 3) Create parser */
    handle->enc.parser = gst_element_factory_make("h264parse", "enc-parser");
    CHECK(handle->enc.encoder != NULL, "Failed to allocate h264parse", RET_ERR);

    /* 4) Create caps filter */ 
    handle->enc.out_caps_filter = gst_element_factory_make("capsfilter", "enc-capsfilter");
    CHECK(handle->enc.out_caps_filter != NULL, "Failed to allocate capsfilter", RET_ERR);
    GstCaps* caps = gst_caps_from_string("video/x-h264,stream-format=byte-stream");
    g_object_set(G_OBJECT(handle->enc.out_caps_filter), "caps", caps, NULL);

    /* 5) Add elements */
    gst_bin_add_many(GST_BIN(handle->pipeline), handle->enc.converter, handle->enc.encoder, 
                    handle->enc.parser, handle->enc.out_caps_filter, NULL);
    
    /* 6) Link elements */
    gboolean ret = gst_element_link_many(handle->enc.converter, handle->enc.encoder, 
                            handle->enc.parser, handle->enc.out_caps_filter, NULL);
    CHECK(ret != FALSE, "Failed to link elements in encoding stage", RET_ERR);
    return RET_OK;
}

static int create_output_stage(PipelineHandle *handle, PipelineConfig *pipeline_config) {
    gboolean ret = FALSE;
    /* 1) Create tee splitter */
    handle->out.tee = gst_element_factory_make("tee", "disp-tee");
    CHECK(handle->out.tee != NULL, "Failed to allocate tee element", RET_ERR);

    /* Device path if necessary */
    if (pipeline_config->dev_sink) {
        /* 2.a1) Create dev queue */
        handle->out.dev_queue = gst_element_factory_make("queue", "disp-devqueue");
        CHECK(handle->out.dev_queue != NULL, "Failed to allocate queue element", RET_ERR);

        /* 2.a2) Create V4L2 sink */
        handle->out.dev_sink = gst_element_factory_make("v4l2sink", "disp-devsink");
        CHECK(handle->out.dev_sink != NULL, "Failed to allocate v4l2sink element", RET_ERR);
        g_object_set(G_OBJECT(handle->out.dev_sink), "device", pipeline_config->dev_sink, NULL);
    }
   
    /* Display path */
    /* 2.b1) Create display decoder */
    handle->out.disp_queue = gst_element_factory_make("queue", "disp-dispqueue");
    CHECK(handle->out.disp_queue != NULL, "Failed to allocate queue element", RET_ERR);

    /* 2.b2) Create display decoder */
    handle->out.disp_decoder = gst_element_factory_make("avdec_h264", "disp-decoder");
    CHECK(handle->out.disp_decoder != NULL, "Failed to allocate avdec_h264 element", RET_ERR);

    /* 2.b3) Create display converter */
    handle->out.disp_converter = gst_element_factory_make("videoconvert", "disp-converter");
    CHECK(handle->out.disp_converter != NULL, "Failed to allocate videoconvert element", RET_ERR);

    /* 2.b4) Create display sink */
    handle->out.disp_sink = gst_element_factory_make("autovideosink", "disp-autovideosink");
    CHECK(handle->out.disp_sink != NULL, "Failed to allocate autovideosink element", RET_ERR);
    g_object_set(G_OBJECT(handle->out.disp_sink), "sync", FALSE, NULL);
    
    /* 3) Add elements */
    gst_bin_add(GST_BIN(handle->pipeline), handle->out.tee);
    gst_bin_add_many(GST_BIN(handle->pipeline), handle->out.disp_queue, handle->out.disp_decoder, 
                    handle->out.disp_converter, handle->out.disp_sink, NULL);

    if (pipeline_config->dev_sink) {
        gst_bin_add_many(GST_BIN(handle->pipeline),handle->out.dev_queue, handle->out.dev_sink, NULL);
    }
    
    /* 4) Link elements */
    ret = gst_element_link_many(handle->out.tee, handle->out.disp_queue, handle->out.disp_decoder, 
                                handle->out.disp_converter, handle->out.disp_sink, NULL);
    CHECK(ret != FALSE, "Failed to link elements in output stage: screen sink", RET_ERR);

#ifdef DEBUT_SHOW_CAPS
    debug_print_caps(handle->out.disp_converter, "src");
    debug_print_caps(handle->out.disp_sink, "sink");
#endif

    if (pipeline_config->dev_sink) {
        ret = gst_element_link_many(handle->out.tee, handle->out.dev_queue, handle->out.dev_sink, NULL);
        CHECK(ret != FALSE, "Failed to link elements in output stage: dev sink", RET_ERR);
    }

    return RET_OK;
}

static GstElement* create_caps_filter(const char* type, const char* name, const char* format, 
                                        int width, int height, int fr_num, int fr_denom) {
    GstElement *caps_filter;
    GstCaps *caps;

    /* Create caps filter element */
    caps_filter = gst_element_factory_make("capsfilter", name);
    CHECK(caps_filter != NULL, "Failed to create caps filter element", NULL);
    

    /* Create a caps structure */
    caps = gst_caps_new_simple(type,
                                "format", G_TYPE_STRING, format,
                                "width", G_TYPE_INT, width, 
                                "height", G_TYPE_INT, height,
                                "framerate", GST_TYPE_FRACTION, fr_denom, fr_num,
                                "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                                NULL); 

    /* Set the caps attribute on the capsfilter element*/
    g_object_set(G_OBJECT(caps_filter), "caps", caps, NULL);
    // gst_caps_unref(caps);
    return caps_filter;
}

static int create_shader_pipeline_from_string(PipelineHandle *handle, const char* shader_pipeline) {
    const char* DELIMITERS = "! "; // TODO: Fix will allow accept "shader1 shader2"
    int num_stages = 0;

    char* copy_shader_pipeline = strdup(shader_pipeline);
    char* shader_name_ptr = strtok(copy_shader_pipeline, DELIMITERS);   
    while (shader_name_ptr != NULL) {
        /* Create shader stage*/
        handle->proc.shader_stages[num_stages++] = create_shader(shader_name_ptr);    
        if (handle->proc.shader_stages[num_stages-1] == NULL) {
            ERROR_FMT("Failed to create shader %s", shader_name_ptr);
            free(copy_shader_pipeline);
            return RET_ERR;
        }

        /* Advance to next shader stage*/ 
        shader_name_ptr = strtok(NULL, DELIMITERS);
    }
    free(copy_shader_pipeline);
    return num_stages;
}

const char *shader_string_vertex_default =
    DEFAULT_SHADER_VERSION
    "attribute vec4 a_position;\n"
    "attribute vec2 a_texcoord;\n"
    "varying vec2 v_texcoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = a_position;\n"
    "   v_texcoord = a_texcoord;\n"
    "}\n";

static GstElement* create_shader(const char* shader_name) {
    GstElement *shader;
    
    /* Load shader code. */
    const char* shader_code = get_shader_code(shader_name);
    if (!shader_code) {
        ERROR_FMT("Failed to load shader code for shader [%s]", shader_name);
        return NULL;
    }
    // DEBUG_PRINT_FMT("Shader code; %s \n", shader_code);

    /* Crate shader object and set properties */
    shader = gst_element_factory_make("glshader", NULL); 
    CHECK(shader != NULL, "Failed to create shader element", NULL);
    g_object_set(G_OBJECT(shader), "fragment", shader_code,
                                    "vertex", shader_string_vertex_default, NULL);

    DEBUG_PRINT_FMT("[%s]-[%s] created! \n", shader_name, GST_ELEMENT_NAME(shader));
    return shader;
}


// Only doing this to avoid annoying build warning :0
#ifdef DEBUT_SHOW_CAPS
static void debug_print_caps(GstElement* elem, const char* pad) {
    GstCaps *caps = gst_pad_query_caps(gst_element_get_static_pad(elem, pad), NULL);
    DEBUG_PRINT_FMT("%s caps: %s\n", pad, gst_caps_to_string(caps));
}
#endif
