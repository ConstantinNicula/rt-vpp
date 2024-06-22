#ifndef __CAM_UTILS_H___
#define __CAM_UTILS_H___

struct cam_params {
    /* Frame properties (pixel format & frame dimensions)*/
    int width;
    int height;
    unsigned int pixelformat;

    /* Stream properties (framerate)*/
    int fr_num;
    int fr_denom;  
};

int read_camera_default_params(const char* dev_path, struct cam_params *out_params);

#endif