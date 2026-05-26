#include "AlarmManager.h"
#include "DataModels.h"
#include "StorageManager.h"
#include "TimeEngine.h"
#include "RTC_DS3231.h"
#include "driver/gpio.h"

#define RTC_PIN GPIO_NUM_2
#define OK_BUTTON_PIN GPIO_NUM_4

using namespace TimeEngine;

RTC_DS3231 rtc(15, 14); 
StorageManager storageManager;
AlarmManager alarmManager;
Time currentTime;
uint16_t currentYear;

void setup() {
    Serial.begin(115200);
    uint32_t startWait = millis();
    while (!Serial && (millis() - startWait < 5000)) {
        delay(10);
    }

    // 1. MUST INITIALIZE I2C HARDWARE INSIDE SETUP (Prevents 2041 Garbage Time)
    rtc.begin();

    // --- 2. HARDWARE INTERROGATION ---
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    uint64_t wakeup_pin_mask = 0; 
    
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    }

    // --- 3. GENERATE NUMERICAL DESIGNATION (BITMASKING) ---
    uint8_t boot_state = 0;
    boot_state |= (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) << 0;       // Bit 0 (Decimal 1)
    boot_state |= ((wakeup_pin_mask & (1ULL << RTC_PIN)) > 0) << 1;         // Bit 1 (Decimal 2)
    boot_state |= ((wakeup_pin_mask & (1ULL << OK_BUTTON_PIN)) > 0) << 2;   // Bit 2 (Decimal 4)

    // --- 4. FETCH GLOBAL TIME ---
    uint8_t t_hour = 0, t_min = 0, t_sec = 0, t_date = 0, t_month = 0;
    uint16_t t_year = 0;
    rtc.getTime(t_hour, t_min, t_sec, t_date, t_month, t_year);
    
    Time currentTime = {t_year, t_month, t_date, t_hour, t_min, t_sec};
    uint32_t currentEpoch = convertDate2Epoch(&currentTime);

    // --- 5. THE STATE DISPATCHER ---
    switch (boot_state) {
        
        case 1: // 0b001 - COLD BOOT (Battery Plugged In)
            Serial.println("=== NORMAL BOOT (BATTERY CONNECTED) ===");
            
            // 1. MOUNT LITTLEFS FIRST
            if (storageManager.initFS() < 0) {
                Serial.printf("LittleFS failed to initialise. Attempting disk format.\r\n");
                storageManager.initFS(true);
            }
            
            // 2. GENERATE EVENTS (Now safe to save because FS is mounted)
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
            
            // 3. SET THE RTC TIME (Breadboard test calibration)
            rtc.setTime(0, 30, 0, 21, 5, 26, 4); 
            
            // 4. RECALCULATE EPOCH (Fetch the new 2026 time we just injected)
            rtc.getTime(t_hour, t_min, t_sec, t_date, t_month, t_year);
            Serial.printf("[DEBUG TIME] RTC Raw Output: %02d/%02d/%d %02d:%02d:%02d\n", 
                  t_date, t_month, t_year, t_hour, t_min, t_sec);
                  
            currentTime = {t_year, t_month, t_date, t_hour, t_min, t_sec};
            currentEpoch = convertDate2Epoch(&currentTime);
            
            // 5. REBUILD QUEUE & ARM ALARM
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            alarmManager.programNextAlarm(&rtc);
            Serial.printf("Total Events loaded: %d\n", storageManager.getTotalEvents());
            break;

        case 2: // 0b010 - RTC ALARM PIN
            Serial.println("=== WOKE UP FROM ALARM (RTC INT) ===");
            
            storageManager.initFS();
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            
            // Handle the UI
            if (alarmManager.getMissedCount() > 0) {
                Serial.printf(">> ALARM RINGING! %d Event(s) <<\n", alarmManager.getMissedCount());
                delay(5000); 
            }
            
            // Arm the next alarm
            alarmManager.programNextAlarm(&rtc);
            break;

        case 4: // 0b100 - OK BUTTON PIN
            Serial.println("=== WOKE UP FROM OK BUTTON ===");
            
            // LittleFS and Queue logic completely safely bypassed!
            Serial.printf("Current Time: %02d:%02d\n", t_hour, t_min);
            delay(3000); // Wait a few seconds so user can read the screen
            break;

        default: // Failsafe for unhandled hardware glitches
            Serial.println("=== UNKNOWN WAKEUP STATE ===");
            break;
    }

    // --- 6. GO TO SLEEP ---
    Serial.println("Entering Deep Sleep (RTC PIN ONLY)...");
    
    // 1. Create a mask that strictly ONLY contains the RTC Pin
    uint64_t wake_mask = (1ULL << RTC_PIN);
    
    // 2. Enable ext1 for just this pin on the ESP32-C6
    esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    
    // 1. Turn on the internal pullup
    pinMode(RTC_PIN, INPUT_PULLUP);
    
    // 2. FORCE the C6 to keep the pullup alive during deep sleep
    gpio_hold_en((gpio_num_t)RTC_PIN);
    
    esp_deep_sleep_start();
}

void loop() {

}