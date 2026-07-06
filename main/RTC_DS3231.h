#ifndef RTC_DS3231_H
#define RTC_DS3231_H

#include <Wire.h>
#include "DataModels.h"
#include "TimeEngine.h"

#define DS3231_ADDR 0x68

/**
 * @class RTC_DS3231
 * @brief driver header for DS3231 RTC module
 * @details It has functions that:-
 * - configure the IC to send alarm interrupts @link RTC_DS3231::begin() @endlink, @link RTC_DS3231::configureControlRegister(uint8_t config) @endlink, @link RTC_DS3231::startOscillator() @endlink
 * - set date and time @link RTC_DS3231::setTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint16_t year, uint8_t weekday) @endlink
 * - obtain date and time @link RTC_DS3231::(uint8_t &hour, uint8_t &minute, uint8_t &second, uint8_t &day, uint8_t &month, uint16_t &year) @endlink
 * - set alarm time @link RTC_DS3231::setAlarm(uint8_t hour, uint8_t minute, uint8_t second, uint8_t date) @endlink
 */
class RTC_DS3231 {
  public:
  /**
   * Default constructor for the class used in SystemCtrl
   */
  RTC_DS3231();
  /**
   * Parameterised contructor for the class used to intialise the RTC class with I2C pins
   */
  RTC_DS3231(uint8_t sda, uint8_t scl) {
    this->scl = scl;
    this->sda = sda;
  }
  /**
   * It initialised the I2C communication using the Wire class.<br>
   * It initialised DS3231 to:-
   * - send interrupt upon alarm match by setting INTCN
   * - enable alarm1 by setting A1IE
   * - starts the oscillator by clearing the Oscillator Stop Flag(OSF)
   */
  void begin() {
    Wire.begin(sda, scl);
    Wire.setClock(100000);
    
    // Ensure oscillator is running
    configureControlRegister(0x05);
    startOscillator();
  }

  /**
   * It is used to configure the control register of DS3231(0Eh).
   */
  void configureControlRegister(uint8_t config) {
    writeRegister(0x0E, config);
  }

  /**
   * Starts the oscillator by clearing the OSF bit and initialising seconds register if MSB has 1 which canonically must stay LOW.
   */
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
  
  /**
   * It is used to set date and time of DS3231 by configuring the registers from 00h to 06h
   */
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


  void setTime(Time* t) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    
    Wire.write(decToBcd(t->sec));
    Wire.write(decToBcd(t->min));
    Wire.write(decToBcd(t->hour)); // 24-hour mode assumed
    Wire.write(decToBcd(TimeEngine::getDayOfWeek(t)));
    Wire.write(decToBcd(t->date));
    Wire.write(decToBcd(t->month));
    Wire.write(decToBcd(t->year % 100));
    
    Wire.endTransmission();
  }

  /**
   * It is used to set alarm by setting the Alarm1 registers(07h to 0Ah), and clearing DY/$\overline{DT}$ bit so that only after all field match is the alarm triggered.
   */
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

  /**
   * It is used to get time from DS3231. This function uses referenced variables to obtain date time.
   */
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

  /**
   * It is used to get time from DS3231. This function uses referenced Time struct object.
   */
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
  
  /**
   * It is used to obtain temperature from the internal temperature sensor of DS3231.
   */
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

  /**
   * clear alarm1 flag from status register
   */
  void clearAlarm1() {
    byte control = readRegister(0x0F);
    control &= ~0x01; // Clear A1IE bit
    writeRegister(0x0F, control);
  }

  /**
   * enable alarm1 interrupt enable bit
   */
  void enableAlarm1() {
    byte control = readRegister(0x0E);
    control |= 0x05; // Set A1IE bit
    writeRegister(0x0E, control);
  }

  /**
   * disable alarm1 interrupt enable bit
   */
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