#pragma once

#ifndef APPSTATE_H
#define APPSTATE_H
#include "RTC/RTC_DS3231.h"
#include "Calendar/DataModels.h"
#include "Calendar/StorageManager.h"
#include "Time/TimeEngine.h"
#include "Calendar/AlarmManager.h"
#include "driver/gpio.h"
#include "Display/SSD1306.h"
#include "Serial.h"

#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

// --- RTC INTERRUPT ---
#define RTC_PIN           GPIO_NUM_2   // Safe: Top Right header

// --- UI BUTTONS ---
#define OK_BUTTON_PIN     GPIO_NUM_12  // Safe: Bottom Right header
#define CANCEL_BUTTON_PIN GPIO_NUM_10  // Safe: Bottom Right header
#define UP_BUTTON_PIN     GPIO_NUM_11  // Safe: Bottom Right header
#define DOWN_BUTTON_PIN   GPIO_NUM_13  // Safe: Bottom Left header
#define WIFI_BUTTON_PIN   GPIO_NUM_9  // Safe: Bottom Left header

// --- ACTUATORS ---
#define MOTOR_PIN         GPIO_NUM_21  // Safe: Middle Left header

RTC_DATA_ATTR uint32_t lastAlarmEpoch = 0;

// Helper function to make the Serial Monitor act like a refreshing screen
void clearConsole() {
    // Serial.print("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"); 
    ESP_LOGI("","\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r");
    // Serial.print("\033[2J\033[H"); 
}

struct AppContext {
    RTC_DS3231* rtc;
    AlarmManager* alarm_manager;
    StorageManager* storage_manager;
    Time* currentTime;
    uint32_t* lastAlarmEpoch;
    SSD1306* display;
};

class AppState {
    protected:
        AppContext* app_context;
    public:
        virtual ~AppState() {}
        virtual void onEnter() = 0;
        virtual void onProgress() = 0;
        virtual AppState* handleInput(uint8_t buttonPressed) = 0; 
        virtual void onExit() = 0;                
};

#endif