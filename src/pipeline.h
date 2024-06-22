#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include "gst/gstelement.h"
#include <gst/gst.h>
#include <linux/videodev2.h>
#include "cam_utils.h"

#define MAX_NUM_SHADER_STAGES 8

typedef struct _DecodingStage {
    /* v4l2 source node */
    GstElement* cam_source; 
    /* Enforces correct camera capture settings */ 
    GstElement* cam_caps_filter; 
    /*Optional: only relevant for MJPEG streams which require dedicated decoder*/
    GstElement* decoder;     
    /* Converter section ensures that output of front end pipeline source
     is compatible with `glupload` sink*/
    GstElement* converter;
    GstElement* out_caps_filter; 
} DecodingStage;

typedef struct _ProcessingStage {
    /* Copy buffer host to GPU*/
    GstElement* uploader;
    /* Processing stages, buffer is NULL terminated */
    GstElement* shader_stages[MAX_NUM_SHADER_STAGES + 1];
    /* Copy buffer GPU to host */
    GstElement* downloader;

    /* TO DO ADD SCALER*/
} ProcessingStage;

typedef struct _EncodingStage {
    /* Convert from gl buffer to x264 compatible */
    GstElement* converter;  
    GstElement* encoder;
} EncodingStage;

typedef struct _PipelineHandle {
    GstElement* pipeline;
    /* Actual processing elements */ 
    DecodingStage dec;
    ProcessingStage proc; 
    EncodingStage enc;
} PipelineHandle;


int create_pipeline(CamParams *cam_params, PipelineHandle *handle);

#endif