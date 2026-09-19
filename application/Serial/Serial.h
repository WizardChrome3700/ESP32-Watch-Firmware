#pragma once
#ifndef SERIALCOMMS_H
#define SERIALCOMMS_H

#include "driver/uart.h"
#include "GPIO.h"
#include <cstdio>  // Required for vsprintf
#include <cstdarg> // Required for va_list, va_start, va_end

#define EX_UART_NUM      UART_NUM_2
#define TXD_PIN          (GPIO_NUM_43)
#define RXD_PIN          (GPIO_NUM_44)
#define RX_BUF_SIZE      (1024)

class SerialComms {
    private:
    uart_config_t uart_config;
    
    public:
    SerialComms();
    void begin(uint32_t baud_rate);
    void write(const char* data);
    void writeBytes(const uint8_t* buffer, size_t size);
    void println(const char* data);
    void printf(const char* format, ...);
    void flush();
    void end();
    int available();
    int read();
};

extern SerialComms Serial;

#endif