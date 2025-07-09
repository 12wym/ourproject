#!/bin/bash

# 在收发端关闭can0设备
ip link set can0 down

# 在收发端设置比特率为250Kbps
ip link set can0 type can bitrate 125000

# 在收发端打开can0设备
ip link set can0 up

#cd /home/firefly/ZLMediaKit/release/linux/Debug/ && ./MediaServer -d &

#set -e
#设置cpu频率
echo 2352000 > /sys/devices/system/cpu/cpufreq/policy6/scaling_setspeed

cat /sys/devices/system/cpu/cpufreq/policy6/cpuinfo_cur_freq

echo userspace > /sys/class/devfreq/fdab0000.npu/governor

echo 1000000000 > /sys/class/devfreq/fdab0000.npu/userspace/set_freq

cat /sys/class/devfreq/fdab0000.npu/cur_freq

sleep 1 # 延时10秒

# relu版本
# cd install/rknn_yolov5_demo_Linux/ && ./rknn_yolov5_demo ./model/4-1280.rknn ../../720p60hz.mp4
cd /home/firefly/fly_debug/fly_demo1/build && ./yolov11_thread_pool
# silu版本
# cd install/rknn_yolov5_demo_Linux/ && ./rknn_yolov5_demo ./model/RK3588/yolov5s.rknn ../../720p60hz.mp4
# 使用摄像头
# cd install/rknn_yolov5_demo_Linux/ && ./rknn_yolov5_demo ./model/RK3588/yolov5s-640-640.rknn 0

