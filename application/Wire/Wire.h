#ifndef WIRE_H
#define WIRE_H

#include <cstdint>
#include <cstddef>
#include "driver/i2c_master.h"
#include "GPIO.h"

#define WIRE_MAX_BUFFER 128

class TwoWire{
private:
     i2c_master_bus_handle_t bus_handle = nullptr;
     uint8_t target_addr = 0;
     uint8_t tx_buffer[WIRE_MAX_BUFFER];
     size_t tx_idx =0;

     uint8_t rx_buffer[WIRE_MAX_BUFFER];
     size_t rx_idx = 0;
     size_t rx_len = 0;
     uint32_t speed_hz = 100000;
     bool is_initialized =false; 

public:
    bool begin(uint8_t sda_pin, uint8_t scl_pin ,uint32_t frequency = 100000 , uint8_t port = 0);
    void beginTransmission(uint8_t addr);
    size_t write(uint8_t data);
    size_t write(const uint8_t *data,size_t quantity);
    uint8_t endTransmission(void);
    uint8_t endTransmission(bool);
    uint8_t requestFrom(uint8_t address,size_t quantity);
    uint16_t read(void);
    int available(void);
    void setClock(uint32_t frequency = 100000);
};

extern TwoWire Wire;

#endif

