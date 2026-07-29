#ifndef ADS1256_H
#define ADS1256_H

#include <SPI.h>

class ADS1256 {
public:
    // Constants for ADS1256 Configuration
    enum PGA_Gain : uint8_t { GAIN_1 = 0, GAIN_2 = 1, GAIN_4 = 2, GAIN_8 = 3, GAIN_16 = 4, GAIN_32 = 5, GAIN_64 = 6 };
    enum DataRate : uint8_t { DR_30000 = 0xF0, DR_15000 = 0xE1, DR_7500 = 0xD2, DR_3750 = 0xC1, DR_2000 = 0xB1, DR_1000 = 0xA1, DR_500 = 0x92, DR_100 = 0x82, DR_10 = 0x03 };
    enum MuxRegister : uint8_t { AIN0 = 0, AIN1 = 1, AIN2 = 2, AIN3 = 3, AIN4 = 4, AIN5 = 5, AIN6 = 6, AIN7 = 7, AINCOM = 8 };

    /**
     * 3. Constructor
     * Configures the pins and properties without hardware DRDY or RESET pins.
     */
    ADS1256(uint8_t pin_sck, uint8_t pin_mosi, uint8_t pin_miso, uint8_t pin_cs, 
                     DataRate rate, PGA_Gain gain) {
        _sck = pin_sck;
        _mosi = pin_mosi;
        _miso = pin_miso;
        _cs = pin_cs;
        _drate = rate;
        _gain = gain;
        
        // Define safe SPI settings: max 2MHz for ADS1256 on Mode 1
        _spiSettings = SPISettings(1920000, MSBFIRST, SPI_MODE1);
    }

    /**
     * 1. Initialization Function
     * Activates SPI, applies gain/data-rate parameters, and toggles the analog input buffer.
     */
    bool init(bool enableBuffer) {
        pinMode(_cs, OUTPUT);
        digitalWrite(_cs, HIGH);

        // Hardware SPI setup on ESP32-C6 customized pins
        SPI.begin(_sck, _miso, _mosi, _cs);

        // Wait for device stabilization
        delay(100);
        resetDevice();

        // Configure ADCON Register (PGA and Clock out off)
        // Bits 7-5: Reserved (0), Bits 4-3: CLKOUT off (0x20 to disable), Bits 2-0: PGA Gain
        uint8_t adcon = 0x20 | (_gain & 0x07);
        writeRegister(REG_ADCON, adcon);

        // Configure STATUS Register (Input Buffer control)
        // Bit 2: ACAL (0), Bit 1: BUFEN (Buffer Enable), Bit 0: DRDY (Read Only)
        uint8_t statusVal = enableBuffer ? 0x02 : 0x00;
        writeRegister(REG_STATUS, statusVal);

        // Configure DRATE Register
        writeRegister(REG_DRATE, _drate);

        // Perform internal self-calibration
        sendCommand(CMD_SELFCAL);
        waitDRDY(); 

        return true;
    }

    /**
     * 2a. Read Single Ended Channel
     * Configures multiplexer to measure specified analog pin relative to AINCOM.
     */
    int32_t readSingleEnded(MuxRegister pin) {
        // Positive input = pin, Negative input = AINCOM
        uint8_t mux = (pin << 4) | AINCOM;
        writeRegister(REG_MUX, mux);
        
        // Sync code required after changing multiplexer paths
        sendCommand(CMD_SYNC);
        delayMicroseconds(4); // t11 delay
        sendCommand(CMD_WAKEUP);
        
        return readADCData();
    }

    /**
     * 2b. Read Dual Ended (Differential) Port
     * Configures multiplexer to measure difference between positive and negative pin.
     */
    int32_t readDifferential(MuxRegister pos_pin, MuxRegister neg_pin) {
        uint8_t mux = (pos_pin << 4) | neg_pin;
        writeRegister(REG_MUX, mux);
        
        sendCommand(CMD_SYNC);
        delayMicroseconds(4);
        sendCommand(CMD_WAKEUP);
        
        return readADCData();
    }

private:
    // Pins and configurations
    uint8_t _sck, _mosi, _miso, _cs;
    DataRate _drate;
    PGA_Gain _gain;
    SPISettings _spiSettings;

    // ADS1256 Opcode Commands
    static const uint8_t CMD_WAKEUP  = 0x00;
    static const uint8_t CMD_RDATA   = 0x01;
    static const uint8_t CMD_RREG    = 0x10;
    static const uint8_t CMD_WREG    = 0x50;
    static const uint8_t CMD_SELFCAL = 0xF0;
    static const uint8_t CMD_SYNC    = 0xFC;
    static const uint8_t CMD_RESET   = 0xFE;

    // Register Address Mapping
    static const uint8_t REG_STATUS  = 0x00;
    static const uint8_t REG_MUX     = 0x01;
    static const uint8_t REG_ADCON   = 0x02;
    static const uint8_t REG_DRATE   = 0x03;

    // Software Polling Routine replacing physical DRDY wire 
    void waitDRDY() {
        uint8_t status = 1;
        while ((status & 0x01) != 0) { // Keep polling until Bit 0 falls to 0 (Data Ready)
            status = readRegister(REG_STATUS);
            delayMicroseconds(2); 
        }
    }

    void sendCommand(uint8_t cmd) {
        SPI.beginTransaction(_spiSettings);
        digitalWrite(_cs, LOW);
        SPI.transfer(cmd);
        digitalWrite(_cs, HIGH);
        SPI.endTransaction();
    }

    void writeRegister(uint8_t reg, uint8_t value) {
        SPI.beginTransaction(_spiSettings);
        digitalWrite(_cs, LOW);
        SPI.transfer(CMD_WREG | reg);
        SPI.transfer(0x00); // Writing 1 register (0x00 = 1 byte)
        SPI.transfer(value);
        digitalWrite(_cs, HIGH);
        SPI.endTransaction();
        delayMicroseconds(2);
    }

    uint8_t readRegister(uint8_t reg) {
        SPI.beginTransaction(_spiSettings);
        digitalWrite(_cs, LOW);
        SPI.transfer(CMD_RREG | reg);
        SPI.transfer(0x00); // Reading 1 register
        delayMicroseconds(7); // minimum t6 delay requirement
        uint8_t value = SPI.transfer(0);
        digitalWrite(_cs, HIGH);
        SPI.endTransaction();
        return value;
    }

    int32_t readADCData() {
        waitDRDY(); // Execute software polling loop

        SPI.beginTransaction(_spiSettings);
        digitalWrite(_cs, LOW);
        SPI.transfer(CMD_RDATA);
        delayMicroseconds(7); // t6 delay

        // Retrieve 24 bits of sample data
        uint8_t highByte = SPI.transfer(0);
        uint8_t midByte  = SPI.transfer(0);
        uint8_t lowByte  = SPI.transfer(0);
        digitalWrite(_cs, HIGH);
        SPI.endTransaction();

        // Sign extend 24-bit 2's complement into a standard signed 32-bit integer
        int32_t value = ((int32_t)highByte << 16) | ((int32_t)midByte << 8) | lowByte;
        if (value & 0x00800000) {
            value |= 0xFF000000; 
        }
        return value;
    }

    void resetDevice() {
        SPI.beginTransaction(_spiSettings);
        digitalWrite(_cs, LOW);
        SPI.transfer(CMD_RESET);
        delay(5); // t11 rest period
        digitalWrite(_cs, HIGH);
        SPI.endTransaction();
    }
};

#endif
