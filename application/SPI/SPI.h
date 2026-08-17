#ifndef HAL_SPI_H
#define HAL_SPI_H

#pragma once
#include "driver/spi_master.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_DEVICES 17
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
#define MAX_DEVICES 17
#endif

typedef enum {
    MSBFIRST = 0,
    LSBFIRST = 1
} DataOrder;

typedef enum {
    SPI_MODE0 = 0,
    SPI_MODE1 = 1,
    SPI_MODE2 = 2,
    SPI_MODE3 = 3
} DataMode;

typedef struct {
    // Standard Arduino Members
    uint32_t speedMaximum             = 1000000;
    DataOrder dataOrder               = MSBFIRST;
    DataMode dataMode                 = SPI_MODE0;

    // --- Extra ESP-IDF Members ---
    uint32_t flags                    = 0;
    uint8_t command_bits              = 0;
    uint8_t address_bits              = 0;
    uint8_t dummyBits                 = 0;
    spi_clock_source_t clock_source   = SPI_CLK_SRC_DEFAULT;
    uint16_t duty_cycle_pos           = 0;
    uint16_t cs_ena_pretrans          = 0;
    uint8_t cs_ena_posttrans          = 0;
    uint32_t input_delay_ns           = 0;
    spi_sampling_point_t sample_point = SPI_SAMPLING_POINT_PHASE_0;
    int spics_io_num                  = -1; // Must stay -1 for manual digitalWrite CS lines
    uint8_t queueSize                 = 1;
    transaction_cb_t pre_cb           = nullptr;
    transaction_cb_t post_cb          = nullptr;
} SPISettings;

class SPI_Controller {
private:
    spi_host_device_t host_id;
    spi_bus_config_t  bus_config; // Removed 'const' so it can be initialised in begin()
    spi_device_handle_t handleList[MAX_DEVICES];
    
    // Fixed invalid C++ multi-element direct initialization assignments
    uint8_t dev_list[MAX_DEVICES]; 
    uint8_t dev_status[MAX_DEVICES];
    uint8_t totalDevices = 0;
    bool bus_initialised = false;
    uint8_t* dma_tx_buf = nullptr;
    uint8_t* dma_rx_buf = nullptr;   

public:
    SPI_Controller(spi_host_device_t host_id);
    ~SPI_Controller();

    esp_err_t begin(uint8_t sclk, uint8_t miso, uint8_t mosi, uint8_t cs, spi_dma_chan_t dma_chan = SPI_DMA_CH_AUTO);
    esp_err_t beginTransaction(SPISettings mySettings);

    // 1. Clones Arduino's standard SPI.endTransaction()
    esp_err_t endTransaction();

    // 2. Overload A: Single Byte Fast Transfer (Mimics standard Arduino SPI.transfer)
    uint8_t transfer(uint8_t byte);

    // 3. Overload B: Multi-Byte DMA/Bulk Transfer (Mimics buffer handling)
    void transfer(uint8_t* buffer, size_t size);

    uint8_t* getTxBuffer() { return dma_tx_buf; }
};

extern SPI_Controller SPI;

#endif // HAL_SPI_H
