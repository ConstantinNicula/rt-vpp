#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include "gst/gstelement.h"
#include <gst/gst.h>
#include <linux/videodev2.h>
#include "cam_utils.h"

#define MAX_NUM_SHADER_STAGES 8

typedef struct _PipelineHandle {
    GstElement* pipeline;
    /* Decoding stage elements */
    struct {
        /* V4L2 source node */
        GstElement* cam_source; 
        /* Enforces correct camera capture settings */ 
        GstElement* cam_caps_filter; 
        /* Optional: only relevant for MJPEG streams which require dedicated decoder*/ 
        GstElement* decoder;     
        /* Converter section ensures that output of front end pipeline source
        is compatible with `glupload` sink*/
        GstElement* converter;
        GstElement* out_caps_filter; 
    } dec;

    /* Processing stage elements */
    struct {
        /* Copy buffer host to GPU*/
        GstElement* uploader;
        /* Processing stages, buffer is NULL terminated */
        GstElement* shader_stages[MAX_NUM_SHADER_STAGES + 1];
        /* Copy buffer GPU to host */
        GstElement* downloader;
        /* Video scaler */
        GstElement* scaler;
        GstElement* out_caps_filter;
    } proc;

    /* Encoding stage elements */
    struct {
        /* Convert from glbuffer to H264 compatible format*/
        GstElement* converter; 
        /* H264 encoding*/ 
        GstElement* encoder;
        /* Parser */ 
        GstElement* parser;
        /* output caps filter*/
        GstElement* out_caps_filter;
    } enc;

    /* Debug display stage elements*/
    struct {
        /* Splitter node for two output paths */ 
        GstElement* tee;
        /* Path 1: V4L2 sink */
        GstElement* dev_queue;
        GstElement* dev_sink;

        /* Path 2: Decoding stage H264 & display sink */
        GstElement* disp_queue;
        GstElement* disp_decoder;
        GstElement* disp_converter;
        GstElement* disp_sink; 
    } out;
} PipelineHandle;


typedef struct _PipelineConfig {
    /* Source settings */
    char* dev_src;

    /* Chain of shader stages */
    char *shader_pipeline;
    char *shader_src_folder;

    /* Output dimensions after rescaling */
    int out_width;
    int out_height;

    /* Encoder settings */
    int bitrate;

    /* Sink settings (NULL if not requested) */
    char *dev_sink; 
} PipelineConfig;

void get_default_pipeline_config(PipelineConfig *out_pipeline_config);
int create_pipeline(CamParams *cam_params, PipelineConfig *pipeline_config, PipelineHandle *out_handle);
int play_pipeline(PipelineHandle* handle);

#endif