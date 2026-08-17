#pragma once
#ifndef MAIN_H
#define MAIN_H

#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"

class Main final
{
private:
    ADS1256 adc;
    SSD1306 display;

public:
    // Unified Shared SPI Bus: SCK=7, MOSI=15, MISO=14
    // ADC CS=10. OLED CS=6, DC=5, RES=4.
    Main() : adc(7, 15, 14, 10, ADS1256::DR_1000, ADS1256::GAIN_1), display(7, 15, 6, 5, 4) {}    
    esp_err_t setup(void);
    void loop(void);
    ~Main();
};

Main::~Main() {}

esp_err_t Main::setup(void)
{
    esp_err_t status{ESP_OK};

    // FIX: Initialize the ADC FIRST. 
    // This forces the SPI bus to mount with the MISO pin (23) fully active.
    // if(adc.init(false)) {
    //     ESP_LOGI(LOG_TAG, "ADS1256 Core Mounted on ESP32-C6 SPI2 Bus!");
    // } else {
    //     ESP_LOGE(LOG_TAG, "ADS1256 Mount Failure!");
    //     return ESP_FAIL;
    // }

    // Initialize OLED SECOND. It will safely skip the hardware mount and share the bus.
    display.ssd1306_init();
    display.clearBuffer();
    display.updateDisplay();
    // display.fillRect(15,15,20,20,1);
    display.drawCircle(100,30,20,1);
    display.updateDisplay();

    return status;
}

void Main::loop(void)
{
    while(true)
    {
        // int32_t A0_val = adc.readSingleEnded(ADS1256::AIN0);
        // int32_t A1_val = adc.readSingleEnded(ADS1256::AIN4);
        // int32_t A2_val = adc.readSingleEnded(ADS1256::AIN2);

        // float voltage0 = (float)A0_val * 10.0f / 16777215.0f;
        // float voltage1 = (float)A1_val * 10.0f / 16777215.0f;
        // float voltage2 = (float)A2_val * 10.0f / 16777215.0f;
        // float avg_voltage = (voltage0 + voltage1 + voltage2) / 3.0f;

        // ESP_LOGI(LOG_TAG, "A0: %d, A1: %d, A2: %d | V0: %.3fV, V1: %.3fV | Avg: %.3fV", 
        //          (int)A0_val, (int)A1_val, (int)A2_val, voltage0, voltage1, avg_voltage);

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

#endif // MAIN_H