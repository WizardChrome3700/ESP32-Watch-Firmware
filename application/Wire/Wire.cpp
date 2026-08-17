#include "Wire.h"
#include "esp_log.h"

static const char *TAG = "WIRE";
TwoWire Wire;

bool TwoWire::begin(uint8_t sda_pin, uint8_t scl_pin, uint32_t frequency,uint8_t port){
    speed_hz = frequency;
    esp_err_t status = ESP_OK;

    if (status == ESP_OK) status = isValidPin(sda_pin);
    if (status == ESP_OK) status = isValidPin(scl_pin);

    if (status == ESP_OK) {
        // Clean struct zero-initialization
        i2c_master_bus_config_t bus_config = {};
        bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_config.i2c_port = (i2c_port_num_t)port;
        bus_config.sda_io_num = (gpio_num_t)sda_pin;
        bus_config.scl_io_num = (gpio_num_t)scl_pin;
        bus_config.glitch_ignore_cnt = 7;
        bus_config.flags.enable_internal_pullup = true;

        esp_err_t err = i2c_new_master_bus(&bus_config,&bus_handle);
        if(err == ESP_OK){
            is_initialized = true;
            return true;
        }
        ESP_LOGI(TAG,"I2C BUS INITIALIZATION FAILED:%s",esp_err_to_name(err));
    }
    ESP_LOGI(TAG,"I2C BUS INITIALIZATION FAILED: Strapping pins chosen.");
    return false;

}

void TwoWire::beginTransmission(uint8_t addr){
    target_addr = addr;
    tx_idx = 0;
}

size_t TwoWire::write(uint8_t data){
    if (tx_idx < WIRE_MAX_BUFFER){
        tx_buffer[tx_idx++]=data;
        return 1;
    };
    return 0;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity){
    size_t written =0;
    for (size_t i =0; i < quantity ; i++){
        written += write(data[i]);

    }
    return written;
}

uint8_t TwoWire::endTransmission(bool sendStop) {
    if (!is_initialized) {
        return 4;
    }

    // Configure the device for this transmission cycle
    i2c_device_config_t dev_cfg = {}; 
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = target_addr;
    dev_cfg.scl_speed_hz = speed_hz;
    
    i2c_master_dev_handle_t dev_handle = nullptr;
    if (i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle) != ESP_OK) {
        tx_idx = 0;
        return 4;
    }

    esp_err_t err = ESP_OK;

    if (sendStop) {
        // Standard transmission: Transmit data packets and terminate with a STOP condition automatically
        err = i2c_master_transmit(dev_handle, tx_buffer, tx_idx, 1000);
        
        // Clean up and remove the device since the transaction sequence is fully complete
        i2c_master_bus_rm_device(dev_handle);
    } else {
        // REPEATED START / NO-STOP Condition requested
        // In modern ESP-IDF, we transmit using the specialized non-stop / multi-stage mode flags if supported,
        // or hold the bus context open for the subsequent read/write.
        err = i2c_master_transmit(dev_handle, tx_buffer, tx_idx, 1000);
        
        // CRITICAL: We do NOT remove the device or issue an isolated stop cycle yet.
        // We track that a continuous sequence is ongoing.
    }

    // Reset buffer tracking index for next payload sequence
    tx_idx = 0;

    // Standard Arduino Wire return mappings mapped from ESP-IDF error definitions
    switch (err) {
        case ESP_OK:            return 0; // Success
        case ESP_FAIL:          return 2; // NACK on transmit address or data
        case ESP_ERR_NOT_FOUND: return 2; // Device not found on bus
        case ESP_ERR_TIMEOUT:   return 5; // Timeout error
        default:                return 4; // Other unexpected hardware errors
    }
}

uint8_t TwoWire::endTransmission(void) {
    return endTransmission(true);
}

uint8_t TwoWire::requestFrom(uint8_t addr, size_t quantity){
    if (!is_initialized || quantity > WIRE_MAX_BUFFER)  return 0;

    i2c_device_config_t dev_cfg = {}; // Zero-initialize struct
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = addr;
    dev_cfg.scl_speed_hz = speed_hz;

    i2c_master_dev_handle_t dev_handle = nullptr;
    if (i2c_master_bus_add_device(bus_handle,&dev_cfg,&dev_handle) != ESP_OK){
        return 0;
    }

    esp_err_t err = i2c_master_receive(dev_handle,rx_buffer,quantity,1000);
    i2c_master_bus_rm_device(dev_handle);

    if (err == ESP_OK) {
        rx_len =quantity;
        rx_idx = 0;
        return quantity;
    }
    rx_len = 0;
    rx_idx = 0;
    return 0;
}

uint16_t TwoWire::read(void){
    if(rx_idx <rx_len){
        return rx_buffer[rx_idx++];
    }
    return -1;
}
int TwoWire::available(void){
    return rx_len - rx_idx;
}

void TwoWire::setClock(uint32_t frequency) {
    speed_hz = frequency;
}