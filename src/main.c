#include "cam_utils.h"
#include <stdio.h>

int main(int argc, char** argv) {
    const char* dev = "/dev/video0";
    struct cam_params params  = {0};
    read_camera_default_params(dev, &params);

    return 0;
}
