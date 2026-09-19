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

/**
 * @brief Data structure to hold global context for the OS states.
 * @details It contains:-
 * - pointer to RTC
 * - pointer to AlarmManager
 * - pointer to StorageManager
 * - pointer to Time containing current time from RTC
 * - pointer to the last alarm's time since epoch
 * - pointer to display used
 */
struct AppContext {
    /**
     * @brief pointer to RTC
     * @details pointer to an object of RTC class to obtain time and program alarms
     */
    RTC_DS3231* rtc;
    /**
     * @brief pointer to AlarmManager
     * @details pointer to an object of AlarmManager class to parse events stored in memory to produce alarm queue from where chronology of alarm programming is obtained.
     */
    AlarmManager* alarm_manager;
    /**
     * @brief pointer to StorageManager
     * @details pointer to an object of StorageManager class to obtain events stored in memory and modify the events storage to delete and add events.
     */
    StorageManager* storage_manager;
    /**
     * @brief pointer to Time containing current time from RTC
     * @details pointer to an object of Time class containing current time from RTC
     */
    Time* currentTime;
    /**
     * @brief pointer to the last alarm's time since epoch
     * @details pointer to the last alarm's time since epoch
     */
    uint32_t* lastAlarmEpoch;
    /**
     * @brief pointer to display used
     * @details pointer to SSD1306 display class being used to manage display buffer and draw/erase text and shapes.
     * @note Create an abstract class for Display drivers from which other specific display drivers inherit to make integration seamless
     */
    SSD1306* display;
};

/**
 * @brief abstract class for all states of OS
 * 
 * @details Each OS state needs to have a pointer to the global OS context. They also need to implement the following functions:-
 * - onEnter function which is invoked when we transition into a particular OS state.
 * - onProgress function which is invoked when we are residing in the same OS state as before.
 * - onExit function is invoked when we transition out of a particular OS state.
 * - handleInput function that is invoked when a button press is detected that handles HID functionality and OS state transitions.
 */
class AppState {
    protected:
        /**
         * @brief holds global context for the OS states.
         * @details holds global context for the OS states.
         */
        AppContext* app_context;
    public:
        /**
         * @brief it is used when a state requests it's own exit function due to it finishing it's operational requirements
         * @details it is used when a state requests it's own exit function due to it finishing it's operational requirements
         */
        bool requestExit = false;
        virtual ~AppState() {}
        /**
         * @brief function which is invoked when we transition into a particular OS state.
         * @details function which is invoked when we transition into a particular OS state.
         */
        virtual void onEnter() = 0;
        /**
         * @brief function which is invoked when we are residing in the same OS state as before.
         * @details function which is invoked when we are residing in the same OS state as before.
         */
        virtual void onProgress() = 0;
        /**
         * @brief function is invoked when we transition out of a particular OS state.
         * @details function is invoked when we transition out of a particular OS state.
         */
        virtual AppState* handleInput(uint8_t buttonPressed) = 0; 
        /**
         * @brief function that is invoked when a button press is detected that handles HID functionality and OS state transitions.
         * @details function that is invoked when a button press is detected that handles HID functionality and OS state transitions.
         */
        virtual void onExit() = 0;                
};

#endif