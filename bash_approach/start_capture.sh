#!/bin/bash 

SHADER_PATH=$(pwd)/../shaders/invert_color.glsl 

gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,width=640,height=480!\
    videoconvert ! capsfilter caps="video/x-raw, format=RGBA" ! \
    glupload ! glshader fragment="\"`cat ${SHADER_PATH}`\""! gldownload! \
    videoconvert ! x264enc bitrate=500 tune="zerolatency" speed-preset="superfast" ! avdec_h264 ! \
    videoconvert ! autovideosink

