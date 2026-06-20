#ifndef SYSCON_H
#define SYSCON_H

#include "RTC_DS3231.h"
#include "DataModels.h"
#include "StorageManager.h"
#include "TimeEngine.h"
#include "AlarmManager.h"
#include "driver/gpio.h"
#include "OLED.h"

#define RTC_PIN GPIO_NUM_2
#define OK_BUTTON_PIN GPIO_NUM_7
#define CANCEL_BUTTON_PIN GPIO_NUM_9
#define UP_BUTTON_PIN GPIO_NUM_6
#define DOWN_BUTTON_PIN GPIO_NUM_8
#define WIFI_BUTTON_PIN GPIO_NUM_23
#define MOTOR_PIN GPIO_NUM_4

RTC_DATA_ATTR uint32_t lastAlarmEpoch = 0;

// Helper function to make the Serial Monitor act like a refreshing screen
void clearConsole() {
    Serial.print("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"); 
    Serial.print("\033[2J\033[H"); 
}

struct AppContext {
    RTC_DS3231* rtc;
    AlarmManager* alarm_manager;
    StorageManager* storage_manager;
    Time* currentTime;
    uint32_t* lastAlarmEpoch;
    OLED* display;
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

class HomeState;
class AlarmState;
class WifiState;
class EventListState;
class EventDetailState;

class HomeState : public AppState {
private:
    uint8_t displayedEventIndex;
    const Time* eventTime_pointer;
public:
    HomeState(AppContext* app_context, uint8_t display_event_index) {
        this->app_context = app_context;
        this->displayedEventIndex = display_event_index;
        // FIX 3: No local copies of time or counts! We fetch dynamically on load.
    }

    void onEnter() override {
        clearConsole();
        
        // Fetch fresh data directly from the system context
        Time* t = app_context->currentTime;
        uint8_t alarmCount = app_context->alarm_manager->getAlarmCount();
        const AlarmNode* alarmQueue = app_context->alarm_manager->getAlarmQueue();
        const Event* eventsArray = app_context->storage_manager->getEventsArray();
        uint16_t totalEvents = app_context->storage_manager->getTotalEvents();
        uint8_t missedCount = app_context->alarm_manager->getMissedCount();

        Serial.println("=========================================");
        Serial.println("             [ HOME SCREEN ]             ");
        Serial.println("=========================================");
        Serial.printf(" TIME: %02d:%02d:%02d\r\n", t->hour, t->min, t->sec);
        Serial.printf(" DATE: %02d/%02d/%d\r\n", t->date, t->month, t->year);
        Serial.println("-----------------------------------------");
        
        if (alarmCount > displayedEventIndex) {
            uint16_t currentID = alarmQueue[displayedEventIndex].eventID;
            for(uint16_t i=0; i < totalEvents; i++) {
                if (eventsArray[i].id == currentID) {
                    Serial.printf(" UPCOMING: %s\r\n", eventsArray[i].name);
                    eventTime_pointer = &eventsArray[i].eventTime;
                    this->app_context->display->drawStringCentered(47, eventsArray[i].name, 1);
                    break;
                }
                else {
                    eventTime_pointer = nullptr;
                }
            }
        } else {
            Serial.println(" UPCOMING: None");
            eventTime_pointer = nullptr;
            // this->app_context->display->drawStringCentered(47, "None", 1);
        }
        
        Serial.println("-----------------------------------------");
        if (missedCount > 0) {
            Serial.printf(" *** %d MISSED EVENT(S)! ***\r\n", missedCount);
        } else {
            Serial.println(" No Missed Events.");
        }
        Serial.println("=========================================\r\n");
        char missed_senc[13];
        char battery_senc[5];
        sprintf(missed_senc, "MISSED: %d", missedCount);
        sprintf(battery_senc, "%d%%", 100);
        this->app_context->display->drawStringRight(1, missed_senc, 1);
        this->app_context->display->drawString(1, 1, battery_senc, 1);
        this->app_context->display->drawStringCentered(37, "UPCOMING", 1);
        this->app_context->display->drawStringCentered(48, "meeting with sir", 1); // WIP: to be removed
    }

    void onProgress() override {
        Time* t = app_context->currentTime;
        char time_senc[9];
        sprintf(time_senc, "%02d:%02d:%02d", t->hour, t->min, t->sec);
        this->app_context->display->fillRect(34, 27, 54, 8, 0);
        this->app_context->display->drawStringCentered(27, time_senc, 1);
        this->app_context->display->drawLine(1, 62, 127, 62, 0);
        if(eventTime_pointer == nullptr) {
            this->app_context->display->drawLine(  1,  62,  10, 62, 1);
            this->app_context->display->drawLine( 16,  62,  20, 62, 1);
            this->app_context->display->drawLine( 26,  62,  30, 62, 1);
            this->app_context->display->drawLine( 36,  62,  40, 62, 1);
            this->app_context->display->drawLine( 46,  62,  50, 62, 1);
            this->app_context->display->drawLine( 56,  62,  60, 62, 1);
            this->app_context->display->drawLine( 66,  62,  70, 62, 1);
            this->app_context->display->drawLine( 76,  62,  80, 62, 1);
            this->app_context->display->drawLine( 86,  62,  90, 62, 1);
            this->app_context->display->drawLine( 96,  62, 100, 62, 1);
            this->app_context->display->drawLine(106,  62, 110, 62, 1);
            this->app_context->display->drawLine(116,  62, 120, 62, 1);
        }
        else {
            float progressBarLength = (float)(convertDate2Epoch(eventTime_pointer) - convertDate2Epoch(t))*127.0/(convertDate2Epoch(eventTime_pointer) - *(app_context->lastAlarmEpoch));
            this->app_context->display->drawLine(1, 62, (uint8_t)progressBarLength, 62, 1);
            Serial.println((uint8_t)progressBarLength);
        }
        this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override {
        Serial.println("Clear screen");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

class AlarmState : public AppState {
private:
    uint8_t debug_print_limit;
public:
    AlarmState(AppContext* app_context) {
        this->app_context = app_context;
        debug_print_limit = 1;
    }

    void onEnter() override {
        clearConsole();
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println("!!             ALARM RINGING           !!");
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println(" (Motor Vibrating...)");
        analogWrite(MOTOR_PIN, 128);
        lastAlarmEpoch = convertDate2Epoch(this->app_context->currentTime);
    }

    void onProgress() override {
        if(debug_print_limit) {
            Serial.println("Update frame for animation.");
            debug_print_limit = 0;
        }
    }

    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override;
};

class WifiState : public AppState {
public:
    WifiState(AppContext* app_context) {
        this->app_context = app_context;
    }

    void onEnter() override {
        clearConsole();
        Serial.println("=========================================");
        Serial.println("             [ WIFI MODULE ]             ");
        Serial.println("=========================================");
        Serial.println(" Status: Turning On...");
        Serial.println(" SSID: ESP32-Watch");
        Serial.println(" IP: 192.168.4.1");
        Serial.println("=========================================\r\n");
    }

    void onProgress() override { return; }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override { Serial.println("[SYSTEM] Turning off WiFi Module..."); }
};

class EventListState : public AppState {
protected:
    uint8_t displayedEventIndex;
public:
    EventListState(AppContext* app_context, uint8_t displayed_event_index) {
        this->app_context = app_context;
        this->displayedEventIndex = displayed_event_index;
    }

    void onEnter() override {
        clearConsole();
        Serial.println("=========================================");
        Serial.println("           [ MISSED EVENTS ]             ");
        Serial.println("=========================================");
        
        // Fetch dynamically!
        const Event* eventsArray = app_context->storage_manager->getEventsArray();
        uint16_t totalEvents = app_context->storage_manager->getTotalEvents();
        const AlarmNode* missedQueue = app_context->alarm_manager->getMissedQueue();
        uint8_t missedCount = app_context->alarm_manager->getMissedCount();
        
        if (missedCount == 0) {
            Serial.println("  (List is empty)");
        } else {
            for(uint8_t i = 0; i < missedCount; i++) {
                uint16_t j;
                for(j = 0; j < totalEvents; j++) {
                    if(missedQueue[i].eventID == eventsArray[j].id) break;
                }
                if(j < totalEvents) {
                    if (i == displayedEventIndex) {
                        Serial.printf(" -> %s (%02d:%02d)\r\n", eventsArray[j].name, eventsArray[j].eventTime.hour, eventsArray[j].eventTime.min);
                    } else {
                        Serial.printf("    %s (%02d:%02d)\r\n", eventsArray[j].name, eventsArray[j].eventTime.hour, eventsArray[j].eventTime.min);
                    }
                }
            }
        }
        Serial.println("=========================================\r\n");
    }

    void onProgress() override { return; }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override { Serial.println("Clear screen"); }
};

class EventDetailState : public AppState {
private:
    const Event* event_pointer;
    uint16_t displayedEventID;
public:
    EventDetailState(AppContext* app_context, uint16_t displayed_event_ID) {
        this->app_context = app_context;
        this->displayedEventID = displayed_event_ID;
        
        // FIX 2: Initialize to nullptr to prevent Load Access Faults!
        this->event_pointer = nullptr; 
        
        const Event* eventsArray = this->app_context->storage_manager->getEventsArray();
        uint16_t eventCount = this->app_context->storage_manager->getTotalEvents();
        
        for(uint16_t i = 0; i < eventCount; i++) {
            if(eventsArray[i].id == displayedEventID) {
                event_pointer = &eventsArray[i];
                break;
            }
        }
    }

    void onEnter() override {
        clearConsole();
        Serial.println("=========================================");
        Serial.println("           [ EVENT DETAILS ]             ");
        Serial.println("=========================================");
        if(event_pointer != nullptr) {
            Serial.printf(" Name:    %s\r\n", event_pointer->name);
            Serial.printf(" Time:    %02d:%02d:%02d\r\n", event_pointer->eventTime.hour, event_pointer->eventTime.min, event_pointer->eventTime.sec);
            Serial.printf(" Date:    %02d/%02d/%d\r\n", event_pointer->eventTime.date, event_pointer->eventTime.month, event_pointer->eventTime.year);
            Serial.println("-----------------------------------------");
            Serial.printf(" Details: %s\r\n", event_pointer->details);
        } else {
            Serial.println(" Error: Event Not Found.");
        }
        Serial.println("=========================================\r\n");
    }

    void onProgress() override { return; }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override { Serial.println("Clear screen"); }
};

// ================= INPUT HANDLERS =================

inline AppState* HomeState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) {
        // FIX 2: Add Bounds check so it refuses to transition if the queue is empty
        if(this->app_context->alarm_manager->getAlarmCount() > displayedEventIndex) {
            return new EventDetailState(this->app_context, this->app_context->alarm_manager->getAlarmQueue()[displayedEventIndex].eventID);
        }
        return this;
    }
    else if(buttonPressed == 2) { return this; }
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
    else if(buttonPressed == 5) { return new WifiState(this->app_context); }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

inline AppState* AlarmState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) { return new HomeState(this->app_context, 0); }
    return this;
}

inline void AlarmState::onExit() {
    analogWrite(MOTOR_PIN, 0);
    this->app_context->rtc->getTime(*(this->app_context->currentTime));
    uint32_t currentEpoch = convertDate2Epoch(this->app_context->currentTime);
    this->app_context->alarm_manager->rebuildQueue(this->app_context->storage_manager->getEventsArray(), this->app_context->storage_manager->getTotalEvents(), currentEpoch);
    this->app_context->alarm_manager->programNextAlarm(this->app_context->rtc);
}

inline AppState* WifiState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) { return this; }
    else if(buttonPressed == 2) { return new HomeState(this->app_context, 0); }
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
    else if(buttonPressed == 2) { return new HomeState(this->app_context, 0); }
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
    else if(buttonPressed == 5) { return new WifiState(this->app_context); }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

inline AppState* EventDetailState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) { return this; }
    else if(buttonPressed == 2) {
        if (event_pointer == nullptr) return new HomeState(this->app_context, 0);

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
            return new HomeState(this->app_context, newDisplayedEventIndex);
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
    else if(buttonPressed == 5) { return new WifiState(this->app_context); }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    
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
    uint8_t boot_state;
    AppState* currentState;
    uint32_t screenTimeOut;
    AppContext appContext;
    uint32_t loopStart;
    OLED display;

    public:
    SystemCtrl(uint32_t timeout);
    void init();
    void boot_handler();
    void system_loop();
    uint8_t read_buttons();
    void shutdown_handler();
};

SystemCtrl::SystemCtrl(uint32_t timeout) : rtc(15, 14), display(22, 21, 18, 19, 20), screenTimeOut{timeout} {
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
}

void SystemCtrl::init() {
    rtc.begin();
    rtc.getTime(currentTime);
    appContext.currentTime = &currentTime;
    display.sh1106_init();
    display.clearBuffer();
    Serial.println("\r\n[SYSTEM] Booting...");
            
    if (storageManager.initFS() < 0) {
        Serial.println("[SYSTEM] LittleFS init failed. Formatting...");
        storageManager.initFS(true);
    }

    wakeup_reason = esp_sleep_get_wakeup_cause();
    uint64_t wakeup_pin_mask = 0; 
    
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pin_mask == 0) {
            wakeup_pin_mask = (1ULL << OK_BUTTON_PIN);
        }
    }

    boot_state = 0;
    boot_state |= (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) << 0;       
    boot_state |= ((wakeup_pin_mask & (1ULL << RTC_PIN)) > 0) << 1;         
    boot_state |= ((wakeup_pin_mask & (1ULL << OK_BUTTON_PIN)) > 0) << 2;   
}

void SystemCtrl::boot_handler() {
    uint32_t currentEpoch;
    switch(boot_state) {
        case 1:
            Serial.println("[SYSTEM] Cold Boot (Battery Connected)");
            rtc.getTime(currentTime);
            currentEpoch = convertDate2Epoch(&currentTime);
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            alarmManager.programNextAlarm(&rtc);
            Serial.printf("Total Events loaded: %d\r\n", storageManager.getTotalEvents());
            currentState = new HomeState(&appContext, 0);
            lastAlarmEpoch = convertDate2Epoch(&currentTime);
            break;
        case 2:
            Serial.println("[SYSTEM] Woke up from RTC ALARM");
            rtc.getTime(currentTime);
            currentEpoch = convertDate2Epoch(&currentTime);
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            alarmManager.programNextAlarm(&rtc);
            currentState = new AlarmState(&appContext);
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
    }
}

uint8_t SystemCtrl::read_buttons() {
    if(digitalRead(OK_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(OK_BUTTON_PIN) == LOW) { return 1; }
    }
    else if(digitalRead(CANCEL_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(CANCEL_BUTTON_PIN) == LOW) { return 2; }
    }
    else if(digitalRead(UP_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(UP_BUTTON_PIN) == LOW) { return 3; }
    }
    else if(digitalRead(DOWN_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(DOWN_BUTTON_PIN) == LOW) { return 4; }
    }
    else if(digitalRead(WIFI_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(WIFI_BUTTON_PIN) == LOW) { return 5; }
    }
    else if(digitalRead(RTC_PIN) == LOW) {
        this->appContext.rtc->clearAlarm1();
        return 6;
    }
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
    loopStart = millis();
    while(millis() - loopStart < screenTimeOut) {
        appContext.rtc->getTime(currentTime);
        uint8_t buttonPressed = this->read_buttons();
        currentState->onProgress();
        if(buttonPressed != 0) {
            const char* btnNames[] = {"NONE", "OK", "CANCEL", "UP", "DOWN", "WIFI", "RTC_ALARM"};
            Serial.printf("[INPUT] Button Registered: %s (%d)\r\n", btnNames[buttonPressed], buttonPressed);
            
            while(this->read_buttons() == buttonPressed) { 
                delay(10); 
                currentState->onProgress(); 
            }
            AppState* nextState = currentState->handleInput(buttonPressed);
            if(nextState != currentState) {
                currentState->onExit();
                delete currentState;
                currentState = nextState;
                currentState->onEnter();
            }
            loopStart = millis();
        }
    }
}

void SystemCtrl::shutdown_handler() {
    // appContext.display->clearBuffer();
    // appContext.display->updateDisplay();
    appContext.display->sh1106_shutdown();
    Serial.println("\r\n[SYSTEM] Timeout reached. Entering Deep Sleep...");
    uint64_t wake_mask = (1ULL << RTC_PIN) | (1ULL << OK_BUTTON_PIN);
    esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    gpio_hold_en((gpio_num_t)RTC_PIN);
    gpio_hold_en((gpio_num_t)OK_BUTTON_PIN);
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);
    esp_deep_sleep_start();
}

#endif