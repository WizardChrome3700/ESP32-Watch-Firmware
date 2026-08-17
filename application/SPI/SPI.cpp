#include "SPI.h"
#include "GPIO.h"
#include "freertos/FreeRTOS.h"
#include "esp_heap_caps.h"

SPI_Controller SPI(SPI2_HOST);

SPI_Controller::SPI_Controller(spi_host_device_t host_id) {
    this->host_id = host_id;
    this->totalDevices = 0;
    this->bus_config = {}; 
    for (int i = 0; i < MAX_DEVICES; i++) {
        dev_list[i] = 255;   
        dev_status[i] = 0;   
        handleList[i] = nullptr;
    }
    // this->dma_rx_buf = (uint8_t*)spi_bus_dma_memory_alloc(host_id, 8, MALLOC_CAP_DMA);
    // this->dma_tx_buf = (uint8_t*)spi_bus_dma_memory_alloc(host_id, 180*240*2, MALLOC_CAP_DMA);
    this->dma_rx_buf = (uint8_t*)heap_caps_malloc(8, MALLOC_CAP_DMA);
    this->dma_tx_buf = (uint8_t*)heap_caps_malloc(180 * 240 * 2, MALLOC_CAP_DMA);
}

esp_err_t SPI_Controller::begin(uint8_t sclk, uint8_t miso, uint8_t mosi, uint8_t cs, spi_dma_chan_t dma_chan) {
    esp_err_t status = ESP_OK;

    if(!(this->bus_initialised)) {
        if (status == ESP_OK) status = isValidPin(mosi);
        if (status == ESP_OK) status = isValidPin(sclk);
        if (status == ESP_OK && miso != 255 && miso != (uint8_t)-1) status = isValidPin(miso);
        if (status != ESP_OK) return status;

        bus_config.mosi_io_num = mosi; 
        bus_config.miso_io_num = (miso == 255 || miso == (uint8_t)-1) ? -1 : miso; 
        bus_config.sclk_io_num = sclk;
        bus_config.quadwp_io_num = -1;
        bus_config.quadhd_io_num = -1;
        bus_config.max_transfer_sz = 240 * 240 * 2; 
        bus_config.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS;

        status = spi_bus_initialize(this->host_id, &bus_config, dma_chan);
        if (status != ESP_OK) return status;
        this->bus_initialised = true;
    }

    if(totalDevices < MAX_DEVICES) {
        if (status == ESP_OK) status = isValidPin(cs);
        if (status == ESP_OK) {
            uint8_t i = 0;
            for(; i < totalDevices; i++) {
                if(dev_list[i] == cs) {
                    return status;
                }
            }
            if(i == totalDevices) { // CS hasn't been allocated yet
                dev_list[totalDevices] = cs;
                totalDevices += 1;
            }
        }
        return status;
    }
    else {
        status = ESP_ERR_NOT_FOUND;
        return status;
    }
}

esp_err_t SPI_Controller::beginTransaction(SPISettings mySettings) {
    spi_device_interface_config_t dev_config = {};
    dev_config.command_bits = mySettings.command_bits;
    dev_config.address_bits = mySettings.address_bits;
    dev_config.dummy_bits = mySettings.dummyBits;
    dev_config.mode = mySettings.dataMode;
    dev_config.clock_source = mySettings.clock_source;
    dev_config.clock_speed_hz = mySettings.speedMaximum;

    if (mySettings.dataOrder == LSBFIRST) {
        dev_config.flags |= SPI_DEVICE_BIT_LSBFIRST;
    }

    uint8_t dev_index = 0;
    for(; dev_index < totalDevices; dev_index++) {
        if(dev_list[dev_index] == mySettings.spics_io_num) {
            break;
        }
    }
    if(dev_index == totalDevices) {
        return ESP_ERR_NOT_ALLOWED;
    }
    dev_config.spics_io_num = -1;
    dev_status[dev_index] = true;
    dev_config.flags |= mySettings.flags;
    dev_config.queue_size = mySettings.queueSize;

    if (this->handleList[dev_index] == nullptr) {
        esp_err_t err = spi_bus_add_device(this->host_id, &dev_config, &this->handleList[dev_index]);
        if (err != ESP_OK) return err;
    }

    esp_err_t status = spi_device_acquire_bus(this->handleList[dev_index], portMAX_DELAY);
    if(status == ESP_OK) dev_status[dev_index] = 1; 
    
    return status;
}

esp_err_t SPI_Controller::endTransaction() {
    uint8_t dev_index = 0;
    for(; dev_index < totalDevices; dev_index++) {
        if(dev_status[dev_index]) {
            break;
        }
    }
    // SAFETY FIX: If no device holds the bus, abort gracefully to prevent a panic
    if(dev_index >= totalDevices) {
        return ESP_ERR_INVALID_STATE;
    }
    spi_device_release_bus(handleList[dev_index]);
    dev_status[dev_index] = false;
    return ESP_OK;
}

uint8_t SPI_Controller::transfer(uint8_t byte) {
    uint8_t dev_index = 0;
    for(; dev_index < totalDevices; dev_index++) {
        if(dev_status[dev_index]) {
            break;
        }
    }

    spi_device_handle_t handle = handleList[dev_index];
    spi_transaction_t trans_des = {};
    
    // RESTORED FIX: Route single bytes through the DMA-safe memory allocation
    // dma_tx_buf[0] = byte;
    trans_des.length = 8; 
    trans_des.tx_data[0] = byte;
    trans_des.rx_buffer = dma_rx_buf;
    trans_des.flags |= SPI_TRANS_USE_TXDATA;

    spi_device_polling_transmit(handle, &trans_des);

    return dma_rx_buf[0];
}

void SPI_Controller::transfer(uint8_t* buffer, size_t size) {
    uint8_t dev_index = 0;
    for(; dev_index < totalDevices; dev_index++) {
        if(dev_status[dev_index]) {
            break;
        }
    }

    spi_device_handle_t handle = handleList[dev_index];
    spi_transaction_t trans_des = {};

    trans_des.length = size*8; 
    trans_des.tx_buffer = buffer;
    trans_des.rx_buffer = dma_rx_buf;
    trans_des.flags &= (~SPI_TRANS_USE_TXDATA);

    spi_device_polling_transmit(handle, &trans_des);
}

SPI_Controller::~SPI_Controller() {
    if (dma_tx_buf != nullptr) {
        heap_caps_free(dma_tx_buf);
        dma_tx_buf = nullptr;
    }
    if (dma_rx_buf != nullptr) {
        heap_caps_free(dma_rx_buf);
        dma_rx_buf = nullptr;
    }
}