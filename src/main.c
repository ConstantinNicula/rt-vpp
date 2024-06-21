#include "utils.h"
#include <linux/videodev2.h>

struct cam_params {
    /* Frame properties (pixel format & frame dimensions)*/
    int width;
    int height;

    /* Stream properties (framerate)*/
    int fr_num;
    int fr_denom;  
};

void read_camera_default_params(const char* dev_path, struct cam_params *out_params);

int main(int argc, char** argv) {
    const char* dev = "/dev/video0";
    PRINT_DEBUG_FMT("Opening file: %s \n", dev);
    return 0;
}

void read_camera_default_params(const char* dev_path, struct cam_params *out_params);