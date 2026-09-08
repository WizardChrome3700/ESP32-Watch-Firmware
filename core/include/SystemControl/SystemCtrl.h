#pragma once
#ifndef SYSCON_H
#define SYSCON_H

#include "SystemControl/Calendar/AlarmState.h"
#include "SystemControl/Calendar/CalendarHomeState.h"
#include "SystemControl/Calendar/EventDetailState.h"
#include "SystemControl/Calendar/EventListState.h"

#include "SystemControl/HomeState/HomeState.h"

#include "SystemControl/Settings/SettingsState.h"
#include "SystemControl/Settings/WifiState/WifiState.h"
#include "SystemControl/Settings/GestureRecord/GestureState.h"
#include "SystemControl/Settings/GestureRecord/GestureRecordState.h"
#include "SystemControl/Settings/GestureRecord/GestureSyncState.h"

extern SemaphoreHandle_t spi_mutex;

class HomeState;
class CalendarHomeState;
class AlarmState;
class WifiState;
class EventListState;
class EventDetailState;
class Settings;
class GestureState;

// ================= INPUT HANDLERS =================

inline AppState* HomeState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) {
        switch(this->cursorIndex) {
            case 0:
            return new CalendarHomeState(this->app_context, 0);
            case 1:
            return new Settings(this->app_context);
            default:
            return this;
        }
    }
    else if(buttonPressed == 2) { return this; }
    else if(buttonPressed == 3) {
        if(cursorIndex == 0) {
            cursorIndex = (this->appCount) - 1;
        }
        else {
            cursorIndex--;
        }
        Serial.printf("cursor index: %d, app count: %d\r\n", cursorIndex, appCount);
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 4) {
        cursorIndex = (cursorIndex + 1) % appCount;
        Serial.printf("cursor index: %d, app count: %d\r\n", cursorIndex, appCount);
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 5) { return this; }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

inline AppState* Settings::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1){
        switch(this-> cursorIndex){
            case 0:
            return new WifiState(this->app_context);
            case 1:
            return new GestureState(this->app_context);
            default:
            return this;
        }
    }
    else if(buttonPressed == 3){//UP Button
        cursorIndex = (cursorIndex + 1) % appCount;
        Serial.printf("cursor index: %d, app count: %d\r\n", cursorIndex, appCount);
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 4){//DOWN Button
        cursorIndex = (cursorIndex + appCount - 1) % appCount;
        Serial.printf("cursor index: %d, app count: %d\r\n", cursorIndex, appCount);
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 2) {
        return new HomeState(this->app_context, 0);
    }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

inline AppState* CalendarHomeState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) {
        // FIX 2: Add Bounds check so it refuses to transition if the queue is empty
        if(this->app_context->alarm_manager->getAlarmCount() > displayedEventIndex) {
            return new EventDetailState(this->app_context, this->app_context->alarm_manager->getAlarmQueue()[displayedEventIndex].eventID);
        }
        return this;
    }
    else if(buttonPressed == 2) { return new HomeState(this->app_context, 0); }
    else if(buttonPressed == 3) {
        uint8_t alarmCount = this->app_context->alarm_manager->getAlarmCount();
        if(alarmCount > 1) {
            // Replaced '% 2' with '% alarmCount' so you can cycle through ALL upcoming alarms
            displayedEventIndex = (displayedEventIndex + 1) % 2;
        }
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 4) { return new EventListState(this->app_context, 0); }
    else if(buttonPressed == 5) { return this; }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

inline AppState* AlarmState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) { return new CalendarHomeState(this->app_context, 0); }
    return this;
}

inline void AlarmState::onExit() {
    analogWrite(MOTOR_PIN, 0);
    this->app_context->display->clearBuffer();
    this->app_context->display->updateDisplay();
    this->app_context->rtc->getTime(*(this->app_context->currentTime));
    uint32_t currentEpoch = convertDate2Epoch(this->app_context->currentTime);
    this->app_context->alarm_manager->rebuildQueue(this->app_context->storage_manager->getEventsArray(), this->app_context->storage_manager->getTotalEvents(), currentEpoch);
    this->app_context->alarm_manager->programNextAlarm(this->app_context->rtc);
    if(animFile) {
        animFile.close();
        delete[] animFrame;
        animFrame = nullptr;
        Serial.println("freed allocated bytes.");
    }
}

inline AppState* WifiState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) { return this; }
    else if(buttonPressed == 2) { return new CalendarHomeState(this->app_context, 0); }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

inline AppState* EventListState::handleInput(uint8_t buttonPressed) {
    uint8_t missed_event_count = this->app_context->alarm_manager->getMissedCount();
    
    if(buttonPressed == 1) {
        if(missed_event_count > displayedEventIndex) {
            return new EventDetailState(this->app_context, (this->app_context->alarm_manager->getMissedQueue()[displayedEventIndex]).eventID);
        }
        return this;
    }
    else if(buttonPressed == 2) { return new CalendarHomeState(this->app_context, 0); }
    else if(buttonPressed == 3) {
        if(missed_event_count != 0) {
            displayedEventIndex = (displayedEventIndex + 1) % missed_event_count;
        }
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 4) {
        if(missed_event_count != 0) {
            displayedEventIndex = (displayedEventIndex + missed_event_count - 1) % missed_event_count;
        }
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 5) { return this; }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

inline AppState* EventDetailState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) { return this; }
    else if(buttonPressed == 2) {
        if (event_pointer == nullptr) return new CalendarHomeState(this->app_context, 0);

        Time event_time = event_pointer->eventTime;
        uint8_t newDisplayedEventIndex = 0; 
        
        if(compareDateTime(&event_time, this->app_context->currentTime) >= 0) {
            const AlarmNode* alarmQueue = this->app_context->alarm_manager->getAlarmQueue();
            uint8_t alarmQueueCount = this->app_context->alarm_manager->getAlarmCount();
            for(uint8_t i = 0; i < alarmQueueCount; i++) {
                if(alarmQueue[i].eventID == displayedEventID) {
                    newDisplayedEventIndex = i;
                    break;
                }
            }
            return new CalendarHomeState(this->app_context, newDisplayedEventIndex);
        }
        else {
            const AlarmNode* missedQueue = this->app_context->alarm_manager->getMissedQueue();
            uint8_t missedQueueCount = this->app_context->alarm_manager->getMissedCount();
            for(uint8_t i = 0; i < missedQueueCount; i++) {
                if(missedQueue[i].eventID == displayedEventID) {
                    newDisplayedEventIndex = i;
                    break;
                }
            }
            return new EventListState(this->app_context, newDisplayedEventIndex);
        }
    }
    else if(buttonPressed == 3) { return this; }
    else if(buttonPressed == 4) { return this; }
    else if(buttonPressed == 5) { return this; }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    
    return this;
}

inline AppState* GestureState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) {
        // 1. Check if the cursor is on the last option ("USB Sync")
        if (cursorIndex == 5) {
            return new GestureSyncState(this->app_context);
        } 
        // 2. Otherwise, start recording the selected gesture
        else {
            return new GestureRecordState(this->app_context, labels[cursorIndex]); 
        }
    }
    else if(buttonPressed == 2) { return new Settings(this->app_context); }
    else if(buttonPressed == 3) { 
        uint8_t label_count = sizeof(labels)/sizeof(labels[0]);
        cursorIndex = (cursorIndex + 1) % label_count;
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 4) {
        uint8_t label_count = sizeof(labels)/sizeof(labels[0]);
        cursorIndex = (cursorIndex + label_count - 1) % label_count;
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 5) { return this; }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

AppState* GestureRecordState::handleInput(uint8_t buttonPressed) {
    // Button 2 (CANCEL) or the 3-second timeout will trigger the exit
    Serial.printf("button_pressed: %d\r\n", buttonPressed);
    if(buttonPressed == 2) { 
        return new GestureState(this->app_context); 
    }
    return this;
}

// ================= SYSTEM CONTROLLER =================

class SystemCtrl {
    private:
    Time currentTime;
    RTC_DS3231 rtc;
    StorageManager storageManager;
    AlarmManager alarmManager;
    esp_sleep_wakeup_cause_t wakeup_reason;
    uint32_t wakeup_mask;
    uint8_t boot_state;
    AppState* currentState;
    uint32_t screenTimeOut;
    AppContext appContext;
    uint32_t loopStart;
    SSD1306 display;
    AppState* stateStack[5];    // Holds up to 5 nested menus
    uint8_t stackPointer;       // Tracks your depth

    public:
    SystemCtrl(uint32_t timeout);
    void init();
    void boot_handler();
    void system_loop();
    uint8_t read_buttons();
    void shutdown_handler();
};

SystemCtrl::SystemCtrl(uint32_t timeout) : rtc(38, 39), screenTimeOut{timeout}, display(7, 15, 6, 5, 4) {
// SystemCtrl::SystemCtrl(uint32_t timeout) : screenTimeOut{timeout}, display(7, 15, 6, 5, 4) {
    pinMode(OK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);
    pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
    pinMode(WIFI_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RTC_PIN, INPUT_PULLUP);
    currentState = nullptr;
    appContext.alarm_manager = &alarmManager;
    appContext.rtc = &rtc;
    appContext.storage_manager = &storageManager;
    appContext.display = &display;
    appContext.lastAlarmEpoch = &lastAlarmEpoch;
    currentState = nullptr;
    stackPointer = 0;
    for(int i=0; i<5; i++) stateStack[i] = nullptr;
}

void SystemCtrl::init() {
    rtc.begin();
    rtc.getTime(currentTime);
    appContext.currentTime = &currentTime;
    display.ssd1306_init();
    display.clearBuffer();
    Serial.println("\r\n[SYSTEM] Booting...");
    // Show message on display instead
            
    if (storageManager.initFS() < 0) {
        ESP_LOGE("SYS", "LittleFS mount failed. Forcing format...");
        // Serial.println("[SYSTEM] LittleFS init failed. Formatting...");
        storageManager.initFS(true);
    }

    wakeup_mask = esp_sleep_get_wakeup_causes();
    uint64_t wakeup_pin_mask = 0;
    if (wakeup_mask & (1 << ESP_SLEEP_WAKEUP_EXT1)) {
        wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pin_mask == 0) {
            wakeup_pin_mask = (1ULL << OK_BUTTON_PIN);
        }
    }

    wakeup_reason = esp_sleep_get_wakeup_cause();
    // uint64_t wakeup_pin_mask = 0;
    // if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    //     wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    //     if (wakeup_pin_mask == 0) {
    //         wakeup_pin_mask = (1ULL << OK_BUTTON_PIN);
    //     }
    // }

    boot_state = 0;
    boot_state |= (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) << 0;       
    boot_state |= ((wakeup_pin_mask & (1ULL << RTC_PIN)) > 0) << 1;         
    boot_state |= ((wakeup_pin_mask & (1ULL << OK_BUTTON_PIN)) > 0) << 2;   
}

void SystemCtrl::boot_handler() {
    uint32_t currentEpoch;

    if (currentState != nullptr) {
        delete currentState;
        currentState = nullptr;
    }

    switch(boot_state) {
        case 1:
            Serial.println("[SYSTEM] Cold Boot (Battery Connected)");
            rtc.getTime(currentTime);
            currentEpoch = convertDate2Epoch(&currentTime);
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            alarmManager.programNextAlarm(&rtc);
            Serial.printf("Total Events loaded: %d\r\n", storageManager.getTotalEvents());
            currentState = new HomeState(&appContext, 0);
            // lastAlarmEpoch = convertDate2Epoch(&currentTime);
            break;
        case 2:
            Serial.println("[SYSTEM] Woke up from RTC ALARM");
            rtc.getTime(currentTime);
            currentEpoch = convertDate2Epoch(&currentTime);
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            alarmManager.programNextAlarm(&rtc);
            // currentState = new AlarmState(&appContext);
            currentState = new HomeState(&appContext, 0);
            break;
        case 4:
            Serial.println("[SYSTEM] Woke up from OK BUTTON");
            rtc.getTime(currentTime);
            currentEpoch = convertDate2Epoch(&currentTime);
            
            // FIX 1: Rebuild the queue here so amnesia is cured!
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            alarmManager.programNextAlarm(&rtc);
            
            currentState = new HomeState(&appContext, 0);
            break;
        default:
            currentState = new HomeState(&appContext, 0);
            break;
    }
    if (currentState == nullptr) {
        currentState = new HomeState(&appContext, 0);
    }
}

uint8_t SystemCtrl::read_buttons() {
    // 1. Static variables keep their values between function calls
    static uint32_t lastDebounceTime = 0;
    static uint8_t lastButtonState = 0;
    
    // 2. Read the raw hardware states instantly without pausing
    uint8_t currentButton = 0;
    if(digitalRead(OK_BUTTON_PIN) == LOW) currentButton = 1;
    else if(digitalRead(CANCEL_BUTTON_PIN) == LOW) currentButton = 2;
    else if(digitalRead(UP_BUTTON_PIN) == LOW) currentButton = 3;
    else if(digitalRead(DOWN_BUTTON_PIN) == LOW) currentButton = 4;
    else if(digitalRead(WIFI_BUTTON_PIN) == LOW) currentButton = 5;
    else if(digitalRead(RTC_PIN) == LOW) {
        this->appContext.rtc->clearAlarm1();
        currentButton = 6;
    }

    // 3. The Non-Blocking Filter
    // If the button changed (pressed or released), reset the timer
    if (currentButton != lastButtonState) {
        lastDebounceTime = esp_timer_get_time() / 1000;
    }

    // 4. Update the state tracker for the next frame
    lastButtonState = currentButton;

    // 5. If the button has been held steady for 50ms, it is a valid press!
    if ((esp_timer_get_time() / 1000) - lastDebounceTime > 50) {
        return currentButton;
    }

    // If we haven't reached 50ms, return 0 (no valid press yet)
    return 0; 
}

void SystemCtrl::system_loop() {
    currentState->onEnter();
    gpio_hold_dis((gpio_num_t)OK_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)CANCEL_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)UP_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)DOWN_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)WIFI_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)RTC_PIN);
    loopStart = esp_timer_get_time() / 1000;
    uint32_t lastRefresh = esp_timer_get_time() / 1000;
    while(esp_timer_get_time() / 1000 - loopStart < screenTimeOut) {
        appContext.rtc->getTime(currentTime);
        uint8_t buttonPressed = this->read_buttons();
        // ADD THIS: Rate Limit the UI rendering to ~30Hz (33ms)
        uint32_t now = esp_timer_get_time() / 1000;
        if (now - lastRefresh >= 33) {
            if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                currentState->onProgress();
                xSemaphoreGive(spi_mutex);
            }
            lastRefresh = now;
        }
        if(buttonPressed != 0) {
            const char* btnNames[] = {"NONE", "OK", "CANCEL", "UP", "DOWN", "WIFI", "RTC_ALARM"};
            Serial.printf("[INPUT] Button Registered: %s (%d)\r\n", btnNames[buttonPressed], buttonPressed);
            
            while(this->read_buttons() == buttonPressed) { 
                vTaskDelay(pdMS_TO_TICKS(10));
                if ((esp_timer_get_time() / 1000) - lastRefresh >= 33) {
                    currentState->onProgress(); 
                    lastRefresh = esp_timer_get_time() / 1000;
                }
            }
            AppState* nextState = currentState->handleInput(buttonPressed);
            if(nextState != currentState) {
                // Lock the bus for the heavy menu transitions
                if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    currentState->onExit();
                    delete currentState;
                    currentState = nextState;
                    currentState->onEnter();
                    xSemaphoreGive(spi_mutex);
                }
            }
            loopStart = esp_timer_get_time() / 1000;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void SystemCtrl::shutdown_handler() {
    if(currentState != nullptr) {
        currentState->onExit();
    }
    appContext.display->clearBuffer();
    appContext.display->updateDisplay();
    appContext.display->ssd1306_shutdown();
    Serial.println("\r\n[SYSTEM] Timeout reached. Entering Light Sleep...");
    uint64_t wake_mask = (1ULL << RTC_PIN) | (1ULL << OK_BUTTON_PIN);
    esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    gpio_hold_en((gpio_num_t)RTC_PIN);
    gpio_hold_en((gpio_num_t)OK_BUTTON_PIN);
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);
    esp_light_sleep_start();

    // ==========================================
    // WAKE UP SEQUENCE
    // ==========================================
    Serial.println("\r\n[SYSTEM] Woke up from Light Sleep!");

    gpio_hold_dis((gpio_num_t)RTC_PIN);
    gpio_hold_dis((gpio_num_t)OK_BUTTON_PIN);

    // 1. RESTORE INTERNAL PULL-UPS (The fix for the infinite loop)
    pinMode(OK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);
    pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
    pinMode(WIFI_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RTC_PIN, INPUT_PULLUP);

    // 2. TRAP THE WAKEUP PRESS (Prevents immediate false clicks)
    while(digitalRead(OK_BUTTON_PIN) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // 3. Re-evaluate the wakeup reason
    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    
    boot_state = 0; // Clear old state
    boot_state |= ((wakeup_pin_mask & (1ULL << RTC_PIN)) > 0) << 1;         
    boot_state |= ((wakeup_pin_mask & (1ULL << OK_BUTTON_PIN)) > 0) << 2;  

    // 4. Call your safe boot_handler!
    this->boot_handler();

    // 5. Power up the screen
    appContext.display->ssd1306_init();
    appContext.display->clearBuffer();
    appContext.display->updateDisplay();

    loopStart = esp_timer_get_time() / 1000;
}

#endif