#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "cam_utils.h"
#include "log_utils.h"

static const char* map_pix_fmt_to_str[__PIX_FMT_MAX] = {
    [PIX_FMT_YUY2] = "YUY2",
    [PIX_FMT_MJPG] = "MJPG", 
    [PIX_FMT_ERROR] = "ERROR"
};
const char* pixel_format_to_str(CamPixelFormat fmt) {
    if (fmt < 0 || fmt >= __PIX_FMT_MAX) 
        return map_pix_fmt_to_str[PIX_FMT_ERROR];
    return map_pix_fmt_to_str[fmt];
}

static void debug_print_fourcc(unsigned int pixel_format) {
    DEBUG_PRINT_FMT("V4L2 Pixel Format: %c%c%c%c\n", 
                    pixel_format & 0xFF,
                    (pixel_format >> 8) & 0xFF, 
                    (pixel_format >> 16) & 0xFF, 
                    (pixel_format >> 24) & 0xFF);
}

static CamPixelFormat convert_v4l2_pixelformat(unsigned int pixel_format) {
    debug_print_fourcc(pixel_format);
    switch(pixel_format) {
        case V4L2_PIX_FMT_YUYV: return PIX_FMT_YUY2;
        case V4L2_PIX_FMT_MJPEG: return PIX_FMT_MJPG;
        default: 
            ERROR("Unrecognized pixel format!");
            return PIX_FMT_ERROR;
    }
}

int read_cam_params(const char* dev_path, CamParams *out_params) {
    struct v4l2_format fmt = {0};
    struct v4l2_streamparm strprm = {0};

    out_params->dev_path = strdup(dev_path);

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
    out_params->pixelformat = convert_v4l2_pixelformat(fmt.fmt.pix.pixelformat);
    
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

    DEBUG_PRINT_FMT("Capture device with default params: width=%d, height=%d, pixelformat=%s, framerate=%d/%d\n", 
        out_params->width, out_params->height, pixel_format_to_str(out_params->pixelformat), out_params->fr_denom, out_params->fr_num);

    close(dev_fd);
    return 0;
}

void cleanup_cam_params(CamParams *params) {
    free(params->dev_path);
}