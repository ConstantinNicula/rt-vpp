# Setup Instructions

## 0. Install dependencies

Gstreamer is the backbone of the processing pipeline so it must be installed, on Ubuntu/Debian you can run the following command:

```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

Some other optional (nice to have utilities):

```bash
sudo apt install v4l-utils
```

## 1. Finding a V4L2 device node

Use v4l2-ctl to find a list of valid devices:

```console
ctin@ctin-VirtualBox:~$ v4l2-ctl --list-devices
Logitech StreamCam (usb-0000:03:00.0-2):
    /dev/video0
    /dev/video1
    /dev/media0
```

If multiple `/dev/video<x>` devices are associated with a given camera, you can use v4l2-ctl commands to figure which `/dev/video<x>` device should be used.
A valid `/dev/video<x>` device should list multiple capture formats MJPEG/YUYV with different resolutions and framerates:

```console
vctin@ctin-VirtualBox:~$ v4l2-ctl -d /dev/video0 --list-formats-ext
ioctl: VIDIOC_ENUM_FMT
    Type: Video Capture

    [0]: 'YUYV' (YUYV 4:2:2)
        Size: Discrete 640x480
            Interval: Discrete 0.033s (30.000 fps)
            Interval: Discrete 0.042s (24.000 fps)
            Interval: Discrete 0.050s (20.000 fps)
            Interval: Discrete 0.067s (15.000 fps)
```

Checking default video capture format:

```console
ctin@ctin-VirtualBox:~$ v4l2-ctl -V -d /dev/video0
Format Video Capture:
    Width/Height      : 640/480
    Pixel Format      : 'YUYV' (YUYV 4:2:2)
    Field             : None
    Bytes per Line    : 1280
    Size Image        : 614400
    Colorspace        : sRGB
    Transfer Function : Rec. 709
    YCbCr/HSV Encoding: ITU-R 601
    Quantization      : Default (maps to Limited Range)
    Flags             : 
```
