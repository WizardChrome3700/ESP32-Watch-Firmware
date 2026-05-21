#include "AlarmManager.h"
#include "DataModels.h"
#include "TimeEngine.h"
#include "StorageManager.h"
#include <Wire.h>
#include "RTC_DS3231.h"

using namespace TimeEngine;

// FIXED: Matched to the physical ESP32-C6 I2C pins we mapped out
RTC_DS3231 rtc(15, 14); 
StorageManager storageManager;
AlarmManager alarmManager;

volatile bool alarmTriggered = false;

// FIXED: The ISR now strictly sets a flag and exits.
void IRAM_ATTR rtcISR() {
    alarmTriggered = true;
}

void setup() {
    Serial.begin(115200);
    
    switch (storageManager.initFS()) {
        case -1:
            Serial.printf("LittleFS failed to initialise. Attempting disk format.\r\n");
            storageManager.initFS(true);
            break;
        case 0:
            break;
        case 1:
            Serial.printf("calendarHeader.bin doesn't open.\r\n");
            break;
        case 2:
            Serial.printf("active.bin doesn't open.\r\n");
            break;
        case 3:
            Serial.printf("calendarHeader.bin and active.bin don't open.\r\n");
            break;
        default:
            Serial.printf("Unexpected output. Files initialisation failed.\r\n");
            break;
    }

    // Generate test events
    for(uint8_t i = 0; i < 10; i++) {
        Event e = {0};
        snprintf(e.name, sizeof(e.name), "Event%d", i + 1);
        snprintf(e.details, sizeof(e.details), "Event%d_details", i + 1);
        e.eventTime.date = 21;
        e.eventTime.month = 5;
        e.eventTime.year = 2026;
        e.eventTime.hour = 0;
        e.eventTime.min = 30;
        e.eventTime.sec = 0 + 20*i;
        if(e.eventTime.sec >= 60) {
            e.eventTime.min += e.eventTime.sec / 60;
            e.eventTime.sec = e.eventTime.sec % 60;
        }
        e.flags = (i % 6) << 3;
        e.repeatInterval = i + 1;
        
        switch(storageManager.saveEvent(&e)) {
            case -1: Serial.printf("Event limit reached.\r\n"); break;
            case 0:  Serial.printf("Event saved successfully.\r\n"); break;
            case 1:  Serial.printf("Files open but failed to write in active.bin.\r\n"); break;
            case 2:  Serial.printf("Files open but failed to write in calendarHeader.bin.\r\n"); break;
            case 4:  Serial.printf("Failed to open active.bin.\r\n"); break;
            case 8:  Serial.printf("Failed to open calendarHeader.bin.\r\n"); break;
            case 12: Serial.printf("Failed to open both files.\r\n"); break;
            default: Serial.printf("Unrecognised return value. Function fail.\r\n"); break;
        }
    }
    
    rtc.begin();
    rtc.setTime(0, 29, 0, 21, 5, 26, 4); // Set initial time
    
    pinMode(2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(2), rtcISR, FALLING);
    
    // FIXED: Prime the alarm queue on boot-up so the first event is loaded into the RTC
    Time currentTime;
    uint16_t currentYear;
    rtc.getTime(currentTime.hour, currentTime.min, currentTime.sec, currentTime.date, currentTime.month, currentYear);
    currentTime.year = currentYear;
    uint32_t currentEpoch = convertDate2Epoch(&currentTime);
    
    const Event* eventArrayPointer = storageManager.getEventsArray();
    alarmManager.rebuildQueue(eventArrayPointer, storageManager.getTotalEvents(), currentEpoch);
    alarmManager.programNextAlarm(&rtc);
    Serial.printf("Total Events loaded: %d\n", storageManager.getTotalEvents());

    // // --- THE 5-SECOND HARDWARE BYPASS TEST ---
    // Wire.beginTransmission(0x68);
    // Wire.write(0x07);    // Start at Alarm 1 Seconds
    // Wire.write(0x05);    // 5 Seconds 
    // Wire.write(0x29);    // 29 Minutes
    // Wire.write(0x00);    // 0 Hours
    // Wire.write(0x21);    // Date 21 (Notice the top bits are naturally 0 here!)
    // Wire.endTransmission();
    
    // rtc.clearAlarm1();
    // rtc.enableAlarm1();
    // Serial.println("Bypass Armed. Waiting 5 seconds...");
}

void loop() {
    // FIXED: Process the alarm securely outside of the hardware interrupt
    if (alarmTriggered) {
        alarmTriggered = false; // Acknowledge the flag
        
        Serial.println("\n--- HARDWARE INTERRUPT TRIGGERED ---");
        
        Time currentTime;
        uint16_t currentYear;
        rtc.getTime(currentTime.hour, currentTime.min, currentTime.sec, currentTime.date, currentTime.month, currentYear);
        currentTime.year = currentYear;
        // FIXED: Passed address of struct
        uint32_t currentEpoch = convertDate2Epoch(&currentTime); 
        
        // At this exact moment, you would trigger your OLED UI and haptic motor!
        
        // Rebuild and push the next event down to the RTC
        const Event* eventArrayPointer = storageManager.getEventsArray();
        
        // FIXED: Added object prefixes
        alarmManager.rebuildQueue(eventArrayPointer, storageManager.getTotalEvents(), currentEpoch); 
        alarmManager.programNextAlarm(&rtc); 
    }
    // Time currentTime;
    // uint16_t currentYear;
    // rtc.getTime(currentTime.hour, currentTime.min, currentTime.sec, currentTime.date, currentTime.month, currentYear);
    // Serial.printf("Current Time: %d-%02d-%02d, %02d:%02d:%02d\r\n", currentYear, currentTime.month, currentTime.date, currentTime.hour, currentTime.min, currentTime.sec);
}