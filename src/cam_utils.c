#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "cam_utils.h"
#include "log_utils.h"


int read_camera_default_params(const char* dev_path, struct cam_params *out_params) {
    struct v4l2_format fmt = {0};
    struct v4l2_streamparm strprm = {0};

    DEBUG_PRINT_FMT("Opening device %s..\n", dev_path);
    int dev_fd = open(dev_path, O_RDWR);
    if (dev_fd == -1) {     
        ERROR_FMT("Failed to open device %s: ", dev_path);
        return -1;
    } 
    /* Read the current frame format */ 
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(dev_fd, VIDIOC_G_FMT, &fmt) == -1) {
        ERROR("IOCTL: VIDIOC_G_FMT request failed \n");
        close(dev_fd);
        return -1;
    }

    /* Extract pixel format info */
    out_params->width = fmt.fmt.pix.width;
    out_params->height = fmt.fmt.pix.height;
    out_params->pixelformat = fmt.fmt.pix.pixelformat;

    /* Read framerate info*/
    strprm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(dev_fd, VIDIOC_G_PARM, &strprm) == -1) {
        ERROR("IOCTL: VIDIOC_G_PARM request failed \n");
        close(dev_fd);
        return -1;
    }

    /* Extract framerate info*/
    out_params->fr_denom = strprm.parm.capture.timeperframe.denominator;
    out_params->fr_num = strprm.parm.capture.timeperframe.numerator;

    DEBUG_PRINT_FMT("Capture device with default params: width=%d, height=%d, pixelformat=%d, framerate=%d/%d\n", 
        out_params->width, out_params->height, out_params->pixelformat, out_params->fr_denom, out_params->fr_num);

    close(dev_fd);
    return 0;
}