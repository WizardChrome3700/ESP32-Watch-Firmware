#pragma once
#ifndef SERIALCOMMS_H
#define SERIALCOMMS_H

#include "driver/uart.h"
#include "GPIO.h"

#define EX_UART_NUM      UART_NUM_2
#define TXD_PIN          (GPIO_NUM_1)
#define RXD_PIN          (GPIO_NUM_2)
#define RX_BUF_SIZE      (1024)

class SerialComms {
    private:
    uart_config_t uart_config;
    
    public:
    SerialComms();
    void begin(uint32_t baud_rate);
    void write(const char* data);
    
    void flush();
    void end();
    int available();
    int read();
};

extern SerialComms Serial;

#endif