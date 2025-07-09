#ifndef _CAN_H_
#define _CAN_H_

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdio.h>
#include <iostream>
#include <vector>
class CAN{
private:
    int s;
    int s_receive;
    int nbytes_send;
    int nbytes_receive;
    struct sockaddr_can addr;
    struct sockaddr_can addr_receive;
    struct ifreq ifr;
    struct ifreq ifr_receive;
    struct can_frame frame[2] = {{0}};
    struct can_frame frame_receive;
    struct can_filter rfilter[1];
    bool isRunning = true;
    int x_value[4] = {0};
    int tag = 0;

public:
    int can_send_init();
    int can_receive_init();
    int can_init();
    
    void receiveThread();

    void check_distance_value();
    void check_camera_value();
    void check_track_value();

    void distance_Return_once();
    void distance_Return_con();


};

#endif