// #include "SystemCtrl.h"

// SystemCtrl sysctl(15000);

// void setup() {
//     Serial.begin(115200);
//     sysctl.init();
//     sysctl.boot_handler();
//     sysctl.system_loop();
//     sysctl.shutdown_handler();
// }

// void loop() {

// }

#include "ADS1256.h"

// Passing explicit custom pins based on your accessible exposed pads (Assuming 23 for MISO)
// SCK=22, MOSI=21, MISO=23, CS=9
ADS1256 adc(22, 21, 23, 9, ADS1256::DR_1000, ADS1256::GAIN_1);
// ADS1256(uint8_t pin_sck, uint8_t pin_mosi, uint8_t pin_miso, uint8_t pin_cs
void setup() {
    Serial.begin(115200);
    // Initialize: true turns on internal buffer for high-impedance sensors
    if(adc.init(false)) {
        Serial.println("ADS1256 initialized via software polling!");
    }
}

void loop() {
    // Read Single ended AIN0 pin (referenced to AINCOM)
    int32_t single_val = adc.readSingleEnded(ADS1256::AIN0);
    
    // Read Differential across Port AIN2 (Positive) and AIN3 (Negative)
    // int32_t diff_val = adc.readDifferential(ADS1256::AIN2, ADS1256::AIN3);

    Serial.printf("Single AIN0: %d\r\n", single_val);
    delay(500);
}