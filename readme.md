# RT-VPP

A configurable real-time video processing pipeline based on GStreamer which allows you to apply 'arbitrarily complex' transformations to camera feeds. The resulting feeds are H.264 encoded and can be directly viewed or optionally routed to v4l2loopback devices. This, in turn, allows other applications to consume the H.264 encoded video, essentially creating a new 'virtual camera'.

## Implementation details

The main idea was to leverage the glshader nodes available in GStreamer to perform the heavy lifting of processing the decoded frames. OpenGL GLSL fragment shaders are used to apply per-frame transformations. Each shader implements anything from a basic transformation to a complex effect. These shaders can be chained to create more complex compound effects. This allows the GPU to handle the grunt work of applying per-pixel transformation, freeing up CPU resources.

The full processing pipeline contains four stages:

- decode stage: responsible with converting the capture stream into a video/x-raw RBGA stream, maintaining framerate and resolution
- processing stage: applies a series of per-frame transformations and resizes frames
- encoding stage: generates the H.264 byte-stream  
- output stage: decodes H264 stream and outputs to a autovideosink and optionally routes the byte stream to a v4l2sink

## Demo

A single v4l2 capture device was duplicated five times using v4l2loopback devices. For each of the five feeds a separate (independent) rt-vpp instance was used to process the video stream. Different configurations were used to showcase the shader effects in action. Refer to the `start_demo.sh` script for more info.

[![Watch the video](https://github.com/ConstantinNicula/rt-vpp/blob/main/demo/thumbnail.png)](https://github.com/ConstantinNicula/rt-vpp/blob/main/demo/demo_short.mp4)

Note: The non-trivial shader effects are slightly modified versions of existing shadertoy shaders. Modifications are limited to adjustments for uniform & varying shader attributes which are not directly compatible. I've added a link in the shader source files to the original shadertoy page.

## Usage

Once you have built the project (refer to Dependencies and Building sections), you can run `./build/rt-vpp --help` to get a full rundown of how to use the rt-vpp command line utility.

```console_
ctin@ctin-VirtualBox:~/workspace/rt-vpp$ ./build/rt-vpp --help
Usage:
  rt-vpp [OPTION?] RealTime Video Processing Pipeline

Help Options:
  -?, --help                                Show help options
  --help-all                                Show all help options
  --help-gst                                Show GStreamer Options

Application Options:
  -p, --shader-pipeline=SHADER_PIPELINE     String which specifies the chain of shaders which should be applied to input stream
                                                (default: vertical_flip ! invert_color)
                                                Example: 'horizontal_flip ! invert_color ! crt_effect'

  -i, --dev-src=SRC_DEVICE                  String which specifies the path to the V4L2 capture device
                                                Example -i /dev/video<x> --dev-src=/dev/video<x>
  -o, --dev-sink=SINK_DEVICE                String which specifies the path to the V4L2 loopback device
                                                Example: -o /dev/video<y> --out-device=/dev/video<y>

  -w, --out-width=OUTPUT_WIDTH              Integer which specifies the width of the scaled output video (default: <input_width>)
                                                Example: -w 800 or  --out-width=800
  -h, --out-height=OUTPUT_HEIGHT            Integer which specifies the height of the scaled output video (default: <input_height>)
                                                Example: -h 600 or --out-height=600

  --bitrate=BITRATE                         Integer which specifies the bitrate of the h264 encoded stream (default: 2000)
                                                Example: --bitrate=1000
  --shader-src-path=SHADER_SRC_PATH         String which specifies the path to shaders source directory (default: ./shaders)
                                                Example: --shader-src-path=../shaders
```

Note: All defined transformation have an associated shader which can be found in `<clone-repo-path>/shaders`. All the shaders found in this folder are loaded at startup and can be used in user defined pipelines. There is a direct mapping between the shader code file name and the transformation name. For example the shader code for transformation `invert_color` can be found in `shaders/invert_color.glsl`.  

### Typical use-cases

1) Read from capture device `/dev/video0`, invert the colors, apply a vertical flip, scale 800x600:

    ```bash
    ./build/rt-vpp -i /dev/video0 -p "invert_color ! horizontal_flip" -w 800 -h 600
    ```

2) Read from capture device `/dev/video0`, apply vignette, apply crt effect, output to loop back device `/dev/video2`:

    ```bash
    ./build/rt-vpp -i /dev/video0 -p "vignette ! crt_effect" -o /dev/video2
    ```

    Refer to section 'Extras/Displaying from a V4L2 loopback device' node for more info about how you can preview the `/dev/video2` feed.

## Dependencies

[Mandatory] Gstreamer is the backbone of the processing pipeline so it must be installed, on Ubuntu/Debian you can run the following command (Note: this is a full installation, not all plugins are necessary but I was too lazy to manually check what the minimal config is):

```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

[Recommended] OpenGL shaders are exclusively used for all frame processing steps. An easy way to check that your platform supports OpenGL is via glxgears util:

```bash
sudo apt-get install mesa-utils
glxgears 
```

[Recommended] This guide uses v4l2-ctl in order to identify/set capture devices properties:

```bash
sudo apt install v4l-utils
```

[Optional] if you intend to use v4l2sink functionally to create an output virtual camera you will need to install a few extra things:

```bash
sudo apt install v4l2loopback-dkms
```

## Building

Building from source should be straightforward once the dependencies have been installed. Assuming you have `make` and `gcc` installed, navigate to the root of the cloned repo and run make:

```bash
cd <cloned-repo-path>
make
```

If the build succeeds you binary named `rt-vpp` is generated in `<clone-repo-path>/build/`.

## Extras

### Finding a V4L2 capture device

Use v4l2-ctl to find a list of valid capture devices:

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

### Creating a V4L2 loopback device

This will be used as the output for the pipeline. You can create loopback device by running the following command:

```bash
sudo modprobe v4l2loopback devices=1
```

Check `/dev/video*` for new devices.

### Displaying from a V4L2 loopback device

You can preview the virtual camera data using gstreamer:

```bash
gst-launch-1.0 v4l2src device=/dev/video2 ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false
```

Or alternatively using ffplay (Note: I haven't figure out how to remove ffplay buffering delays):

```bash
ffplay /dev/video2
```
