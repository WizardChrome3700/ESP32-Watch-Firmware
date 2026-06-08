#ifndef RTC_DS3231_H
#define RTC_DS3231_H

#include <Wire.h>
#include "DataModels.h"

#define DS3231_ADDR 0x68

class RTC_DS3231 {
  public:
  RTC_DS3231();
  RTC_DS3231(uint8_t sda, uint8_t scl) {
    this->scl = scl;
    this->sda = sda;
  }
  void begin() {
    Wire.begin(sda, scl);
    Wire.setClock(100000);
    
    // Ensure oscillator is running
    configureControlRegister(0x05);
    startOscillator();
  }

  void configureControlRegister(uint8_t config) {
    writeRegister(0x0E, config);
  }

  bool startOscillator() {
    // Clear OSF bit in status register
    byte status = readRegister(0x0F);
    if(status & 0x80) {
      writeRegister(0x0F, status & 0x7F);
    }
    
    // Clear CH bit in seconds register
    byte seconds = readRegister(0x00);
    if(seconds & 0x80) {
      writeRegister(0x00, seconds & 0x7F);
    }
    
    // Verify
    delay(10);
    return ((readRegister(0x0F) & 0x80) == 0);
  }
  
  void setTime(uint8_t hour, uint8_t minute, uint8_t second,
               uint8_t day, uint8_t month, uint16_t year,
               uint8_t weekday) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    
    Wire.write(decToBcd(second));
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour)); // 24-hour mode assumed
    Wire.write(decToBcd(weekday));
    Wire.write(decToBcd(day));
    Wire.write(decToBcd(month));
    Wire.write(decToBcd(year % 100));
    
    Wire.endTransmission();
  }


  void setAlarm(uint8_t hour, uint8_t minute, uint8_t second, uint8_t date) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x07);
    Wire.write(decToBcd(second));
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour)); // 24-hour mode assumed
    Wire.write(decToBcd(date));
    Wire.endTransmission();

    // Wire.beginTransmission(DS3231_ADDR);
  }

  void getTime(uint8_t &hour, uint8_t &minute, uint8_t &second,
               uint8_t &day, uint8_t &month, uint16_t &year) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    Wire.endTransmission(false);
    uint8_t weekday; // Unused but required for reading the full time
    Wire.requestFrom(DS3231_ADDR, 7);
    if(Wire.available() >= 7) {
      second = bcdToDec(Wire.read() & 0x7F);
      minute = bcdToDec(Wire.read());
      hour = bcdToDec(Wire.read() & 0x3F);
      weekday = bcdToDec(Wire.read());
      day = bcdToDec(Wire.read());
      month = bcdToDec(Wire.read() & 0x1F);
      year = bcdToDec(Wire.read()) + 2000;
    }
  }

  void getTime(Time &currentTime) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    Wire.endTransmission(false);
    uint8_t weekday; // Unused but required for reading the full time
    Wire.requestFrom(DS3231_ADDR, 7);
    if(Wire.available() >= 7) {
      currentTime.sec = bcdToDec(Wire.read() & 0x7F);
      currentTime.min = bcdToDec(Wire.read());
      currentTime.hour = bcdToDec(Wire.read() & 0x3F);
      uint8_t weekday = bcdToDec(Wire.read());
      currentTime.date = bcdToDec(Wire.read());
      currentTime.month = bcdToDec(Wire.read() & 0x1F);
      currentTime.year = bcdToDec(Wire.read()) + 2000;
    }
  }
  
  float getTemperature() {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x11);
    Wire.endTransmission(false);
    
    Wire.requestFrom(DS3231_ADDR, 2);
    if(Wire.available() >= 2) {
      int8_t msb = Wire.read();
      uint8_t lsb = Wire.read();
      
      float temp = msb;
      if(lsb & 0x80) temp += 0.75;
      else if(lsb & 0x40) temp += 0.50;
      else if(lsb & 0x20) temp += 0.25;
      
      return temp;
    }
    return -999.0;
  }

  void clearAlarm1() {
    byte control = readRegister(0x0F);
    control &= ~0x01; // Clear A1IE bit
    writeRegister(0x0F, control);
  }

  void enableAlarm1() {
    byte control = readRegister(0x0E);
    control |= 0x05; // Set A1IE bit
    writeRegister(0x0E, control);
  }

  void disableAlarm1() {
    byte control = readRegister(0x0E);
    control &= ~0x01; // Clear A1IE bit
    writeRegister(0x0E, control);
  }

  private:
  uint8_t sda;
  uint8_t scl;
  byte readRegister(byte reg) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(DS3231_ADDR, 1);
    return Wire.available() ? Wire.read() : 0xFF;
  }
  
  void writeRegister(byte reg, byte value) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
  }
  
  byte bcdToDec(byte val) { return ((val / 16) * 10 + (val % 16)); }
  byte decToBcd(byte val) { return ((val / 10) * 16 + (val % 10)); }
};

#endif