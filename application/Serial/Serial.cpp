#include "Serial.h"

SerialComms::SerialComms() {
    ;
}

void SerialComms::begin(uint32_t baud_rate) {
    this->uart_config = {};
    // Replaced hardcoded 115200 to utilize the passed baud_rate parameter
    uart_config.baud_rate = baud_rate; 
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    
    // Apply configuration to the selected UART hardware instance
    ESP_ERROR_CHECK(uart_param_config(EX_UART_NUM, &uart_config));

    /* 2. Assign GPIO pins for TX and RX */
    ESP_ERROR_CHECK(uart_set_pin(EX_UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    /* 3. Install the UART driver with a ring buffer for incoming data */
    ESP_ERROR_CHECK(uart_driver_install(EX_UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
}

void SerialComms::write(const char* data) {
    uint8_t len = 0;
    for(len = 0; *(data + len) != '\0'; len++);
    uart_write_bytes(EX_UART_NUM, (const char *) data, len);
}

void SerialComms::flush() {
    // Blocks execution until the TX hardware FIFO is completely empty
    uart_wait_tx_done(EX_UART_NUM, portMAX_DELAY);
}

void SerialComms::end() {
    // Uninstalls the UART driver and frees the allocated ring buffers
    uart_driver_delete(EX_UART_NUM);
}

int SerialComms::available() {
    // Queries the RX ring buffer for the number of bytes waiting to be read
    size_t length = 0;
    uart_get_buffered_data_len(EX_UART_NUM, &length);
    return (int)length;
}

int SerialComms::read() {
    uint8_t data;
    // Reads 1 byte with a 0-tick timeout (non-blocking)
    int len = uart_read_bytes(EX_UART_NUM, &data, 1, 0); 
    if (len > 0) {
        return data;
    }
    return -1; // Standard Arduino behavior: return -1 if buffer is empty
}

SerialComms Serial;