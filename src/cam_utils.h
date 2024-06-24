#ifndef __CAM_UTILS_H___
#define __CAM_UTILS_H___

typedef enum {
    PIX_FMT_YUY2,
    PIX_FMT_RGB24, 
    PIX_FMT_BGR24,
    PIX_FMT_I420, 
    PIX_FMT_MJPG, 
    PIX_FMT_ERROR,
    __PIX_FMT_MAX
} CamPixelFormat;

typedef struct _CamParams {
    /* Source device */
    char* dev_path;

    /* Frame properties (pixel format & frame dimensions)*/
    CamPixelFormat pixelformat;
    int width;
    int height;

    /* Stream properties (framerate)*/
    int fr_num;
    int fr_denom;  
} CamParams;

const char* pixel_format_to_str(CamPixelFormat fmt);
int read_cam_params(const char* dev_path, CamParams *out_params);
void cleanup_cam_params(CamParams* params);

#endif