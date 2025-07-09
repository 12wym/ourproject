//
// Created by Yuan on 2023/12/1.
//

#include "uart.h"
#include <vector>
#include <atomic>
std::atomic<uint16_t> distance[2];
/**
 * 串口初始化
*/
int Uart::init_uart()
{
    const char* serialPort = "/dev/ttyS0";  // 串口设备路径，根据实际情况更改

    // 打开串口
    this->serialPortF = open(serialPort, O_RDWR | O_NOCTTY | O_NDELAY);
    if (this->serialPortF == -1) {
        std::cerr << "Error: Unable to open serial port!" << std::endl;
        return -1;
    }

    // 配置串口参数
    struct termios tty;
    if (tcgetattr(this->serialPortF, &tty) != 0) {
        std::cerr << "Error: Could not get serial port attributes!" << std::endl;
        return -1;
    }

    cfsetospeed(&tty, B115200);  // 波特率 9600，可以根据需要调整
    cfsetispeed(&tty, B115200);

    tty.c_cflag &= ~PARENB;  // 无校验位
    tty.c_cflag &= ~CSTOPB;  // 一个停止位
    tty.c_cflag &= ~CSIZE;   // 8 位数据位
    tty.c_cflag |= CS8;

    tty.c_cflag &= ~CRTSCTS;  // 禁用硬件流控制

    tty.c_cflag |= CREAD | CLOCAL;  // 启用读取和本地连接

    // 设置输入模式，忽略回车和换行符
    tty.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);

    // 设置输出模式，忽略回车和换行符
    tty.c_oflag &= ~OPOST;

    // 设置本地模式
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

    // 设置字符间隔和最小字符数量
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 5;

    if (tcsetattr(this->serialPortF, TCSANOW, &tty) != 0) {
        std::cerr << "Error: Could not set serial port attributes!" << std::endl;
        return -1;
    }


    return 0;


}

void Uart::ReadHexData(){
    for(;;)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int bytesRead = read(this->serialPortF, this->buf, sizeof(this->buf) - 1);
        if (bytesRead > 0) {
            std::cout << bytesRead << std::endl;
            this->buf[bytesRead] = '\0';
            std::cout << "Received data: ";
            printHex(this->buf, bytesRead);
            get_distance();
        }
    }
}

void Uart::get_distance(){
    distance[0].store(this->buf[6]);
    distance[1].store(this->buf[7]);
}




void Uart::sendHexData(const char* hexData) {
    size_t hexDataLen = strlen(hexData);
    size_t byteCount = hexDataLen / 2;

    char* byteData = new char[byteCount];
    for (size_t i = 0; i < byteCount; ++i) {
        sscanf(hexData + 2 * i, "%2hhx", &byteData[i]);
    }

    ssize_t bytesWritten = write(this->serialPortF, byteData, byteCount);
    if (bytesWritten == -1) {
        std::cerr << "Error: Could not write to serial port!" << std::endl;
    } else {
        std::cout << "Sent data: ";
        printHex(byteData, bytesWritten);
    }

    delete[] byteData;
}

void Uart::printHex(const char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(data[i]) & 0xFF) << " ";
    }
    std::cout << std::dec << std::endl;
}


void Uart::Modify_uart_mode(int i){
    if(i == 1)
    {
        //连续测距
        hexDataToSend = "ee1602030407";
        sendHexData(hexDataToSend.c_str());
    }
    else if(i == 2)
    {
        //单次测距
        hexDataToSend = "ee1602030205";
        sendHexData(hexDataToSend.c_str());

    }
    else if(i == 3)
    {
        //停止测距
        hexDataToSend = "ee1602030508";
        sendHexData(hexDataToSend.c_str());
    }
    
}