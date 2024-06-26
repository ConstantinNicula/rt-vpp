#include "cam_utils.h"
#include "glib.h"
#include "gst/gstdebugutils.h"
#include "pipeline.h"

#include "log_utils.h"
#include "shader_utils.h"
#include <gst/gst.h>

static int read_cmd_line_params(int argc, char *argv[], PipelineConfig* out_config); 

int main(int argc, char *argv[]) {
    PipelineHandle handle = {0};
    PipelineConfig pipeline_config = {0};
    CamParams cam_params  = {0};

    /* Parse command line args */
    if (read_cmd_line_params(argc, argv, &pipeline_config) != RET_OK) return RET_ERR;
    DEBUG_PRINT_FMT("MAIN: %s\n", pipeline_config.shader_pipeline);
    /* Initialize shader stuff */
    init_shader_store();
    DEBUG_PRINT_FMT("Loading shaders from %s\n", pipeline_config.shader_src_folder);
    if (add_shaders_to_store(pipeline_config.shader_src_folder) != RET_OK) goto err; 
  
    /* Read camera parameters */
    DEBUG_PRINT_FMT("Reading camera parameters for device %s\n", pipeline_config.dev_src);
    if (read_cam_params(pipeline_config.dev_src, &cam_params) != RET_OK) goto err;
    
    /* Create the elements */
    if (create_pipeline(&cam_params, &pipeline_config, &handle) != RET_OK) goto err; 

    /* Play pipeline */
    if (play_pipeline(&handle) != RET_OK) goto err;

    /* Exit success*/ 
    return RET_OK;

err: 
    /* Clean allocated junk*/
    cleanup_shader_store();
    cleanup_cam_params(&cam_params);
    return RET_ERR;
}  

int read_cmd_line_params(int argc, char *argv[], PipelineConfig* out_config) {
    GOptionContext  *context = NULL;
    GError *error = NULL;

    /* Set defaults*/
    get_default_pipeline_config(out_config);

    /* Define user switches */
    #define INDENT_LEVEL "\t\t\t\t\t\t" // hack but couldn't find a better way
    GOptionEntry entries[] = {
        {"shader-pipeline", 'p', 0, G_OPTION_ARG_STRING, &out_config->shader_pipeline, 
            "String which specifies the chain of shaders which should be applied to input stream\n" 
            INDENT_LEVEL "(default: vertical_flip ! invert_color)\n" 
            INDENT_LEVEL "Example: 'horizontal_flip ! invert_color ! crt_effect'\n", "SHADER_PIPELINE"}, 

        {"dev-src", 'i', 0, G_OPTION_ARG_STRING, &out_config->dev_src, 
            "String which specifies the path to the V4L2 capture device\n" 
            INDENT_LEVEL "Example -i /dev/video<x> --dev-src=/dev/video<x>", "SRC_DEVICE"},
        {"dev-sink", 'o', 0, G_OPTION_ARG_STRING, &out_config->dev_sink, 
            "String which specifies the path to the V4L2 loopback device\n"
            INDENT_LEVEL "Example: -o /dev/video<y> --out-device=/dev/video<y>\n", "SINK_DEVICE"}, 

        {"out-width", 'w', 0, G_OPTION_ARG_INT, &out_config->out_width, 
            "Integer which specifies the width of the scaled output video (default: <input_width>)\n"
            INDENT_LEVEL "Example: -w 800 or  --out-width=800", "OUTPUT_WIDTH"},
        {"out-height", 'h', 0, G_OPTION_ARG_INT, &out_config->out_height, 
            "Integer which specifies the height of the scaled output video (default: <input_height>)\n"
            INDENT_LEVEL "Example: -h 600 or --out-height=600\n", "OUTPUT_HEIGHT"},

       {"bitrate", 0, 0, G_OPTION_ARG_INT, &out_config->bitrate, 
            "Integer which specifies the bitrate of the h264 encoded stream (default: 2000)\n"
            INDENT_LEVEL "Example: --bitrate=1000", "BITRATE"},
        {"shader-src-path", 0, 0, G_OPTION_ARG_STRING, &out_config->shader_src_folder, 
            "String which specifies the path to shaders source directory (default: ./shaders)\n" 
            INDENT_LEVEL "Example: --shader-src-path=../shaders", "SHADER_SRC_PATH"},
       {NULL}
    };

    /* Initialize GStreamer & parse entries */
    context = g_option_context_new("RealTime Video Processing Pipeline"); 
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gst_init_get_option_group());

    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        ERROR_FMT("Failed to initialize: %s", error->message); 
        g_clear_error(&error);
        g_option_context_free(context);
        return RET_ERR;
    }

    g_option_context_free(context);
    return RET_OK;
}
    

