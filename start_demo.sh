#!/bin/bash

# Usage ./strat_demo.sh <input device> <num_streams_to_create> 
# Eg. input_device = /dev/video0 
# Eg. num_streams_to_create = 5 (valid rage: [1..5])

SCRIPT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")"; pwd -P ) 

allocate_devices () {
    local num_devices=$1

    # list devices before allocation
    sudo modprobe -r v4l2loopback
    local ls_before=$(ls /dev/video*)

    # allocate new devices and list
    sudo modprobe v4l2loopback devices=$num_devices
    local ls_after=$(ls /dev/video*) 

    # perform a diff 
    local loopback_devices=$(echo $ls_before $ls_after | tr ' ' '\n' | sort | uniq -u | tr '\n' ' ') 
    echo $loopback_devices
}

start_cature_dup () { 
    local source_device=$1
    local num_devices=$2

    # allocate new output devices 
    shift 
    shift 
    out_devices=("$@")
    # IFS=' ' read -r -a out_devices <<< "$(allocate_devices $num_devices)"

    # set source in gst pipeline 
    local gst_pipeline_cmd="v4l2src device=$source_device ! video/x-raw,framerate=30/1,width=640,height=480 ! tee name=t "

    # for each output device create pipeline str
    for dev in "${out_devices[@]}"; do
        gst_pipeline_cmd+="t. ! queue ! videoconvert !  v4l2sink device=$dev " 
    done

    # launching gst..
    nohup gst-launch-1.0 $gst_pipeline_cmd > splitter.log 2>&1 &
    local pid=$!
    echo $pid
}

sample_pipeline_args=(
    "--shader-pipeline=\"passthrough!crt_effect\" --bitrate=10000"
    "--shader-pipeline=\"vertical_flip!invert_color\" -w 640 -h 480 --bitrate=10000"
    "--shader-pipeline=\"ascii_effect!vignette\" -w 640 -h 480 --bitrate=10000"
    "--shader-pipeline=\"drunk_effect!crt_effect\" -w 640 -h 480 --bitrate=10000"
    "--shader-pipeline=\"chromatical!vignette\" -w 640 -h 480 --bitrate=10000"
    "--shader-pipeline=\"passthrough\" --bitrate=10000"
)

# cleanup previous logs
rm out_*.log

# allocate new devices 
out_devs_str="$(allocate_devices $2)"
echo Created $2 devices: $out_devs_str ..

# start duplicating input stream to new devices
dup_pid=$(start_cature_dup $1 $2 $out_devs_str)
echo Starting GStreamer virtual cam duplication @ PID: $dup_pid

# wait a bit
sleep 1 

# spawn rt-vpp processing instances

echo Start demo pipelines on each device
IFS=' ' read -r -a out_devs_arr <<< "$out_devs_str"

# uncomment to enable debugging
export DISPLAY=:0
export GST_DEBUG=3

pipeline_pids=()
for i in $(seq 0 $(($2 - 1))); do
    # echo Starting processing pipeline $i with params: $cmd_params ..
    nohup ./build/rt-vpp -i ${out_devs_arr[$i]} ${sample_pipeline_args[$i]} --shader-src-path=$SCRIPT_PATH/shaders > out_$i.log 2>&1 &
    echo "nohup ./build/rt-vpp -i ${out_devs_arr[$i]} ${sample_pipeline_args[$i]} --shader-src-path=$SCRIPT_PATH/shaders > out_$i.log 2>&1 &"
    pipeline_pids+=("$!")
done

echo "Press any key to close..."
read

# close opened windows
for pid in "${pipeline_pids[@]}"; do
    kill -9 $pid
done
kill -9 $dup_pid
