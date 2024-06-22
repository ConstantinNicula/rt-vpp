#ifndef __CAM_UTILS_H___
#define __CAM_UTILS_H___

typedef struct _CamParams {
    /* Source device */
    char* dev_path;

    /* Frame properties (pixel format & frame dimensions)*/
    int width;
    int height;
    unsigned int pixelformat;

    /* Stream properties (framerate)*/
    int fr_num;
    int fr_denom;  
} CamParams;


int read_cam_params(const char* dev_path, CamParams *out_params);
void cleanup_cam_params(CamParams* params);

#endif