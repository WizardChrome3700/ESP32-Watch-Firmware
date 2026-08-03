#include "ADS1256.h"

// Passing explicit custom pins based on your accessible exposed pads (Assuming 23 for MISO)
// SCK=22, MOSI=21, MISO=23, CS=9
ADS1256 adc(22, 21, 23, 9, ADS1256::DR_1000, ADS1256::GAIN_1);
// ADS1256(uint8_t pin_sck, uint8_t pin_mosi, uint8_t pin_miso, uint8_t pin_cs
void setup() {
    Serial.begin(115200);
    // Initialize: true turns on internal buffer for high-impedance sensors
    if(adc.init(true)) {
        Serial.println("ADS1256 initialized via software polling!");
    }
}

void loop() {
    // Read Single ended AIN0 pin (referenced to AINCOM)
    int32_t A0_val = adc.readSingleEnded(ADS1256::AIN0);
    int32_t A1_val = adc.readSingleEnded(ADS1256::AIN1);
    int32_t A2_val = adc.readSingleEnded(ADS1256::AIN2);

    float voltage0 = (float)A0_val*10.0/16777215.0;
    float voltage1 = (float)A1_val*10.0/16777215.0;
    float voltage2 = (float)A2_val*10.0/16777215.0;

    float avg_voltage = (voltage0 + voltage1 + voltage2)/3;

    Serial.printf("A0: %d, A1: %d, A2: %d, voltage0: %f, voltage1: %f, voltage2: %f, avg_voltage: %f\r\n", A0_val, A1_val, A2_val, voltage0, voltage1, voltage2, avg_voltage);
    delay(500);
}
