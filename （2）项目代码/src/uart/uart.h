#ifndef _UART_H
#define _UART_H

#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <thread>

class Uart{
    
private:
    int serialPortF = 0;
    char buf[256];
public:
    std::string hexDataToSend = "ee1602030407";  
    int init_uart();
    void printHex(const char* data, size_t length);
    void sendHexData(const char* hexData);
    void ReadHexData();
    void Modify_uart_mode(int i);

    void get_distance();
};


#endif