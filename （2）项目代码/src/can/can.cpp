#include "can.h"
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unistd.h>
extern int can_status = 0;
std::mutex lock_can;
bool data_ready = false;
std::condition_variable cv_can;

extern std::atomic<uint16_t> distance[2];


int CAN::can_send_init(){
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);//创建套接字
    strcpy(ifr.ifr_name, "can0" );
    ioctl(s, SIOCGIFINDEX, &ifr); //指定 can0 设备
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(s, (struct sockaddr *)&addr, sizeof(addr));//将套接字与 can0 绑定
    //禁用过滤规则，本进程不接收报文，只负责发送
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

    //回环
    int loopback = 0; // 0 表示关闭, 1 表示开启( 默认)
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
    //生成两个报文
    frame[0].can_id = 0x86;
    frame[0].can_dlc = 8;
    frame[1].can_id = 0x86;
    frame[1].can_dlc = 5;

    frame[0].data[0] = 0xaa;
    frame[0].data[1] = 0x55;
    frame[0].data[2] = 0x00;
    frame[0].data[3] = 0x40;
    frame[0].data[4] = 0x00;
    frame[0].data[5] = 0x01;
    frame[0].data[6] = 0x00;
    frame[0].data[7] = 0x00;
    
    frame[1].data[0] = 0x00;
    frame[1].data[1] = 0x00;
    frame[1].data[2] = 0x00;
    frame[1].data[3] = 0x00;
    frame[1].data[4] = 0x40;

    return 0;

}

int CAN::can_receive_init(){
    s_receive = socket(PF_CAN, SOCK_RAW, CAN_RAW);//创建套接字
    strcpy(ifr_receive.ifr_name, "can0" );
    ioctl(s_receive, SIOCGIFINDEX, &ifr_receive); //指定 can0 设备
    addr_receive.can_family = AF_CAN;
    addr_receive.can_ifindex = ifr_receive.ifr_ifindex;
    bind(s_receive, (struct sockaddr *)&addr_receive, sizeof(addr_receive));//将套接字与 can0 绑定
    //禁用过滤规则，本进程不接收报文，只负责发送
    setsockopt(s_receive, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);
    //生成两个报文
    rfilter[0].can_id = 0x0553030C;
    //rfilter[0].can_id = 0x01;
    rfilter[0].can_mask = CAN_SFF_MASK;
    //设置过滤规则
    setsockopt(s_receive, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
    return 0;

}


int CAN::can_init(){
    int ret = 0;
    ret = can_send_init();
    ret = can_receive_init();
    return ret;

}

//单次测距返回值
void CAN::distance_Return_once(){
    frame[0].data[0] = 0xaa;
    frame[0].data[1] = 0x55;
    frame[0].data[2] = 0x00;
    frame[0].data[3] = 0x50;
    frame[0].data[4] = 0x01;

    frame[0].data[5] = distance[0];
    frame[0].data[6] = distance[1];
    
    // frame[0].data[5] = 0x11;
    // frame[0].data[6] = 0x11;

    frame[0].data[7] = 0x00;

    frame[1].data[0] = 0x00;
    frame[1].data[1] = 0x00;
    frame[1].data[2] = 0x00;
    frame[1].data[3] = 0x00;
    uint16_t sum = 0;
    for (int i = 0; i < 8; ++i) {
        sum += (frame[0].data[i] & 0xFF);
    }

    frame[1].data[4] = (uint8_t)(sum & 0xFF);
    nbytes_send = write(s, &frame[0], sizeof(frame[0])); //发送 frame[1]
    usleep(1 * 10);  // 延时指定的毫秒数
    nbytes_send = write(s, &frame[1], sizeof(frame[1])); //发送 frame[1]
    std::cout << "nbyte" << nbytes_send << std::endl;
    std::cout << "测距发送成功" << std::endl;
    for(int i = 0 ; i < 8 ; i++)
    {
        printf("%02X ", frame[0].data[i]);

    }
    std::cout << std::endl;
    for(int i = 0 ; i < 5 ; i++)
    {
        printf("%02X ", frame[1].data[i]);

    }
    std::cout << std::endl;
    if (nbytes_send == -1) {
        // 发送操作失败，查看 errno 并输出错误信息
        perror("write");
    }
    if(nbytes_send != sizeof(frame[1]))
    {
        
        printf("Send error!\n");

    }

}





void CAN::check_distance_value(){
    /**
     * 测距命令
    */
    if(frame_receive.data[3] == 0x50 && frame_receive.data[4] == 0x00 && frame_receive.data[5] == 0x00)
    {
        std::cout << "关闭测距" << std::endl;

    }
    if(frame_receive.data[3] == 0x50 && frame_receive.data[4] == 0x00 && frame_receive.data[5] == 0x01)
    {
        std::cout << "单次测距模式" << std::endl;
        distance_Return_once();

        

    }
    if(frame_receive.data[3] == 0x50 && frame_receive.data[4] == 0x00 && frame_receive.data[5] == 0x02)
    {
        std::cout << "连续测距模式" << std::endl;
        distance_Return_once();

    }


}


void CAN::check_camera_value(){
    /**
     * 变焦命令
    */
    if(frame_receive.data[3] == 0x40 && frame_receive.data[4] == 0x00 && frame_receive.data[5] == 0x01)
    {
        std::cout << "近焦---1" << std::endl;
        {
            std::lock_guard<std::mutex> lock(lock_can);
            can_status = 0;
            data_ready = true; // 数据准备好了
        }
        // cv_can.notify_one(); // 通知等待的消费者线程
        
        frame[0].data[3] = 0x40;
        frame[0].data[4] = 0x01;
        frame[0].data[5] = 0x40;
        frame[0].data[6] = 0x40;
        frame[1].data[4] = 0xC0;
        nbytes_send = write(s, &frame[0], sizeof(frame[0])); //发送 frame[1]
        nbytes_send = write(s, &frame[1], sizeof(frame[1])); //发送 frame[1]
        std::cout << "nbyte" << nbytes_send << std::endl;
        if (nbytes_send == -1) {
            // 发送操作失败，查看 errno 并输出错误信息
            perror("write");
        }
        if(nbytes_send != sizeof(frame[1]))
        {
            
            printf("Send error!\n");

        }

    }
    if(frame_receive.data[3] == 0x40 && frame_receive.data[4] == 0x00 && frame_receive.data[5] == 0x02)
    {
        std::cout << "远焦---2" << std::endl;
        {
            std::lock_guard<std::mutex> lock(lock_can);
            can_status = 1;
            data_ready = true; // 数据准备好了
        }
        // cv_can.notify_one(); // 通知等待的消费者线程

        frame[0].data[3] = 0x40;
        frame[0].data[4] = 0x01;
        frame[0].data[5] = 0x40;
        frame[0].data[6] = 0x40;
        frame[1].data[4] = 0xC0;
        nbytes_send = write(s, &frame[0], sizeof(frame[0])); //发送 frame[1]
        nbytes_send = write(s, &frame[1], sizeof(frame[1])); //发送 frame[1]
        std::cout << "nbyte" << nbytes_send << std::endl;
        if (nbytes_send == -1) {
            // 发送操作失败，查看 errno 并输出错误信息
            perror("write");
        }
        if(nbytes_send != sizeof(frame[1]))
        {
            
            printf("Send error!\n");

        }
    }
    if(frame_receive.data[3] == 0x40 && frame_receive.data[4] == 0x00 && frame_receive.data[5] == 0x03)
    {
        std::cout << "红外---3" << std::endl;
        {
            std::lock_guard<std::mutex> lock(lock_can);
            can_status = 2;
            data_ready = true; // 数据准备好了
        }
        // cv_can.notify_one(); // 通知等待的消费者线程

        frame[0].data[3] = 0x40;
        frame[0].data[4] = 0x01;
        frame[0].data[5] = 0x40;
        frame[0].data[6] = 0x40;
        frame[1].data[4] = 0xC0;
        nbytes_send = write(s, &frame[0], sizeof(frame[0])); //发送 frame[1]
        nbytes_send = write(s, &frame[1], sizeof(frame[1])); //发送 frame[1]
        std::cout << "nbyte" << nbytes_send << std::endl;
        if (nbytes_send == -1) {
            // 发送操作失败，查看 errno 并输出错误信息
            perror("write");
        }
        if(nbytes_send != sizeof(frame[1]))
        {
            
            printf("Send error!\n");

        }
    }

}

void CAN::check_track_value(){
    /**
     * 位置信息 , 跟踪模式
     * 1.手动单目标跟踪 -> SDI1进入跟踪(手动框选目标)
     * 2.多目标跟踪视频-> 1080P
     * 3.原始视频 -> 退出跟踪（显示原始视频流）
    */

    if(frame_receive.data[1] == 0x55 && frame_receive.data[3] == 0x26)
    {
            x_value[tag] =  frame_receive.data[4];
            tag++;
    }
    if(frame_receive.data[1] == 0x55 && frame_receive.data[3] == 0x00)
    {
            x_value[tag] =  frame_receive.data[4];
            tag++;
    }

    if(tag == 4){
        std::cout << "进入手动跟踪模式" << std::endl;
        for(int i = 0 ; i < 4 ; i++)
            std::cout << x_value[i] << " ";
        std::cout << std::endl;
        tag = 0;
    }

    if(frame_receive.data[1] == 0x55 && frame_receive.data[3] == 0x29)
    {
        std::cout << "进入原始跟踪模式" << std::endl;
    }
    if(frame_receive.data[1] == 0x55 && frame_receive.data[3] == 0x25)
    {
        std::cout << "退出跟踪，返回原始视频流" << std::endl;
    }



    


}


void CAN::receiveThread() {
    while (isRunning) {
        // 在这里执行接收操作，可以使用 read 函数等
        nbytes_receive = read(s_receive, &frame_receive, sizeof(frame_receive));
        int flag = 0;
        // 在这里处理接收到的数据，例如打印消息信息
        if (nbytes_receive > 0)
        {   
            // std::cout << "                            " << std::endl;
            // std::cout << "Received: ID=" << frame_receive.can_id << std::endl;
            // for(int i = 0 ; i < 13 ; i++)
            // {
            //     std::cout << static_cast<int>(frame_receive.data[i]) << " ";

            // }
            check_distance_value();
            check_camera_value();
            check_track_value();
        }

        for (int i = 0; i < 8; ++i) {
            frame[0].data[i] = 0x00;

        }
        for(int i = 0 ; i < 5 ; ++i)
            frame[1].data[i] = 0x00;

        // 添加其他需要执行的操作

        // 这里可以添加适当的延时，以避免过于频繁的循环
        // 例如： std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10毫秒
    }
}

