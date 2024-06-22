#!/bin/bash 

DEVICE=/dev/video0

# Make exec location invariant 
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")"; pwd -P ) 
SHADER_PATH=$PARENT_PATH/../shaders/invert_color.glsl 

# Extract current resolution and framerate using v4l2-ctl 
CAPTURE_FORMAT=$(v4l2-ctl -d $DEVICE -V)
RESOLUTION=$(echo "$CAPTURE_FORMAT" | sed -rn "s/[[:space:]]*Width\/Height[[:space:]]*:[[:space:]]([[:digit:]]*)\/([[:digit:]]*)/\1 \2/p" )
PIXEL_FORMAT=$(echo "$CAPTURE_FORMAT" | sed -rn "s/[[:space:]]*Pixel Format[[:space:]]*:[[:space:]]*'([[:alpha:]]*)'.*/\1/p" )
WIDTH=$(echo $RESOLUTION | awk '{print $1}')
HEIGHT=$(echo $RESOLUTION | awk '{print $2}')

# Write out information
echo Device: $DEVICE, resolution=$WIDTH/$HEIGHT, pixelfromat=$PIXEL_FORMAT


# Switch on input file format, the front-end of the processing pipeline changes
case $PIXEL_FORMAT in 
    *YUYV ) 
    echo "Using YUYV pipeline...." 
    # Camera input format is raw YUYV
    gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,width=$WIDTH,height=$HEIGHT!\
        videoconvert ! capsfilter caps="video/x-raw, format=RGBA" ! \
        glupload ! glshader fragment="\"`cat ${SHADER_PATH}`\"" ! gldownload! \
        videoconvert ! x264enc bitrate=500 tune="zerolatency" speed-preset="superfast" ! h264parse ! avdec_h264  ! \
        videoconvert ! autovideosink
    ;;

    *MJPG ) 
    echo "Using MJPEG pipeline"
    # Camera input format is MJPG (needs conversion from JPEG to x-raw RGBA) 
    gst-launch-1.0 v4l2src device=/dev/video0 ! image/jpeg,width=$WIDTH,height=$HEIGHT ! \
        avdec_mjpeg ! videoconvert ! capsfilter caps="video/x-raw,width=$WIDTH,height=$HEIGHT,format=RGBA" ! \
        glupload ! glshader fragment="\"`cat ${SHADER_PATH}`\"" ! gldownload! \
        videoconvert ! x264enc bitrate=500 tune="zerolatency" speed-preset="superfast" ! h264parse ! avdec_h264 ! \
        videoconvert ! autovideosink
    ;;

    *)
    echo Unknown input pixel format \'$PIXEL_FORMAT\'
   ;;
esac