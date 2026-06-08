    // System control class
    // It's job is to handle all the external inputs and state transitions
    /**
     * It should initialise the drivers for peripherals.
     * It should enable/disable hold on GPIO pins for external wakeup.
     * It should investigate the wakeup reason.
     * It should run the wakeup event loop.
    */

    // Wakeup event class
    /**
     * It should initialise wakeup event and it's protocols.
    */

#ifndef SYSTEMCTRL_H
#define SYSTEMCTRL_H

#include "RTC_DS3231.h"
#include "DataModels.h"
#include "StorageManager.h"
#include "TimeEngine.h"
#include "AlarmManager.h"
#include "driver/gpio.h"

#define RTC_PIN GPIO_NUM_2
#define OK_BUTTON_PIN GPIO_NUM_5
#define CANCEL_BUTTON_PIN GPIO_NUM_1
#define UP_BUTTON_PIN GPIO_NUM_6
#define DOWN_BUTTON_PIN GPIO_NUM_7
#define WIFI_BUTTON_PIN GPIO_NUM_8
#define MOTOR_PIN GPIO_NUM_4

class AppState {
    public:
        virtual ~AppState() {}
        
        // Called once when the screen is loaded to draw the initial graphics
        virtual void onEnter() = 0;
        virtual void onProgress() = 0;
        
        // Called continuously. Returns a pointer to a new state if a transition happens, 
        // or returns 'this' if we stay on the same screen.
        virtual AppState* handleInput(uint8_t buttonPressed) = 0; 
        
        // Called once when leaving the screen to clean up (e.g., turn off WiFi)
        virtual void onExit() = 0;                
};

// Forward declare all states so pointers work
class HomeState;
class AlarmState;
class WifiState;
class EventListState;
class EventDetailState;

class HomeState : public AppState {
public:
    void onEnter() override {
        // Draw the clock and battery on the OLED
        Serial.println("Print Time, Battery %, Date, current Event, number of missed events, timeout progress bar.");
    }

    void onProgress() override {
        return;
    }

    AppState* handleInput(uint8_t buttonPressed) override;

    void onExit() override {
        // Clear the screen buffer
        Serial.println("Clear screen");
    }
};

class AlarmState : public AppState {
public:
    void onEnter() override {
        Serial.println("Draw start frame of Alarm Screen animation.");
        analogWrite(MOTOR_PIN, 128);
    }

    void onProgress() override {
        Serial.println("Update frame for animation.");
    }

    AppState* handleInput(uint8_t buttonPressed) override;

    void onExit() override {
        // Clear the screen buffer
        analogWrite(MOTOR_PIN, 0);
        Serial.println("Clear screen");
    }
};

class WifiState : public AppState {
public:
    void onEnter() override {
        Serial.println("Turn on Wifi module and display Wifi information like SSID, IP, password.");
    }

    void onProgress() override {
        return;
    }

    AppState* handleInput(uint8_t buttonPressed) override;

    void onExit() override {
        // Clear the screen buffer
        Serial.println("Turn off Wifi module and clear screen");
    }
};

class EventListState : public AppState {
public:
    void onEnter() override {
        Serial.println("Fetch missed event list and display them on screen.");
    }

    void onProgress() override {
        return;
    }

    AppState* handleInput(uint8_t buttonPressed) override;

    void onExit() override {
        // Clear the screen buffer
        Serial.println("Clear screen");
    }
};

class EventDetailState : public AppState {
public:
    void onEnter() override {
        Serial.println("Display missed event details like name, detail string, time, date.");
    }

    void onProgress() override {
        return;
    }

    AppState* handleInput(uint8_t buttonPressed) override;

    void onExit() override {
        // Clear the screen buffer
        Serial.println("Clear screen");
    }
};

inline AppState* HomeState::handleInput(uint8_t buttonPressed) {
        if (buttonPressed == 5) {
            return new WifiState(); // Transition to the WiFi screen
        }
        if (buttonPressed == 4) {
            return new EventListState(); // Transition to Missed Events
        }
        if (buttonPressed == 3) {
            Serial.println("Change event on the screen.");
            return this;
        }
        return this; // No transition, stay on Home
    }

inline AppState* AlarmState::handleInput(uint8_t buttonPressed) {
    if (buttonPressed == 1) {
        return new HomeState(); // Transition to the WiFi screen
    }
    return this; // No transition, stay on Home
}

inline AppState* WifiState::handleInput(uint8_t buttonPressed) {
    if (buttonPressed == 2) {
        return new HomeState(); // Transition to the Home screen
    }
    return this; // No transition, stay on WifiState
}

inline AppState* EventListState::handleInput(uint8_t buttonPressed) {
        if (buttonPressed == 1) {
            return new EventDetailState();
        }
        if (buttonPressed == 2) {
            return new HomeState(); // Transition to the WiFi screen
        }
        if (buttonPressed == 3) {
            Serial.println("Move cursor UP.");
        }
        if (buttonPressed == 4) {
            Serial.println("Move cursor DOWN.");
        }
        if (buttonPressed == 5) {
            return new WifiState();
        }
        return this; // No transition, stay on EventListState
    }

inline AppState* EventDetailState::handleInput(uint8_t buttonPressed) {
        if (buttonPressed == 1) {
            return new EventDetailState();
        }
        if (buttonPressed == 2) {
            return new HomeState(); // Transition to the WiFi screen
        }
        if (buttonPressed == 3) {
            Serial.println("Move cursor UP.");
        }
        if (buttonPressed == 4) {
            Serial.println("Move cursor DOWN.");
        }
        if (buttonPressed == 5) {
            return new WifiState();
        }
        return this; // No transition, stay on EventDetailState
    }

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

    public:
    SystemCtrl(uint32_t timeout);
    void init();
    void boot_handler();
    void system_loop();
    uint8_t read_buttons();
    void shutdown_handler();
};

SystemCtrl::SystemCtrl(uint32_t timeout) : rtc(15, 14), screenTimeOut{timeout} {
    pinMode(OK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);
    pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
    pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RTC_PIN, INPUT_PULLUP);
    currentState = nullptr;
}

void SystemCtrl::init() {
    rtc.begin();
    Serial.println("=== NORMAL BOOT (BATTERY CONNECTED) ===");
            
    // 1. MOUNT LITTLEFS FIRST
    if (storageManager.initFS() < 0) {
        Serial.printf("LittleFS failed to initialise. Attempting disk format.\r\n");
        storageManager.initFS(true);
    }

    wakeup_reason = esp_sleep_get_wakeup_cause();
    uint64_t wakeup_pin_mask = 0; 
    
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        // --- TRANSIENT BUTTON FIX ---
        // If the CPU wakes up but the pin is already released, the mask is 0.
        // Because the RTC alarm holds permanently LOW, a 0 mask GUARANTEES it was the button!
        if (wakeup_pin_mask == 0) {
            wakeup_pin_mask = (1ULL << OK_BUTTON_PIN);
        }
    }

    boot_state = 0;
    boot_state |= (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) << 0;       // Bit 0 (Decimal 1)
    boot_state |= ((wakeup_pin_mask & (1ULL << RTC_PIN)) > 0) << 1;         // Bit 1 (Decimal 2)
    boot_state |= ((wakeup_pin_mask & (1ULL << OK_BUTTON_PIN)) > 0) << 2;   // Bit 2 (Decimal 4)
}

void SystemCtrl::boot_handler() {
    uint32_t currentEpoch;
    switch(boot_state) {
        case 1:
            Serial.println("=== NORMAL BOOT (BATTERY CONNECTED) ===");
            /* Dummy events and time setup */
            // GENERATE EVENTS because event entry is not yet setup (Now safe to save because FS is mounted)
            for(uint8_t i = 0; i < 10; i++) {
                Event e = {0};
                snprintf(e.name, sizeof(e.name), "Event%d", i + 1);
                snprintf(e.details, sizeof(e.details), "Event%d_details", i + 1);
                e.eventTime.date = 21;
                e.eventTime.month = 5;
                e.eventTime.year = 2026;
                e.eventTime.hour = 0;
                e.eventTime.min = 30;
                e.eventTime.sec = 30;
                uint16_t delta = 90*i;
                if(delta + 30 > 59) {
                    e.eventTime.min += (delta + 30) / 60;
                    e.eventTime.sec = (delta + 30) % 60;
                }
                e.flags = (i % 6) << 3;
                e.repeatInterval = i + 1;
                switch(storageManager.saveEvent(&e)) {
                    case -1: Serial.printf("Event limit reached.\r\n"); break;
                    case 0:  Serial.printf("Event saved successfully."); Serial.printf("Saving event at %02d:%02d:%02d\r\n", e.eventTime.hour, e.eventTime.min, e.eventTime.sec); break;
                    case 1:  Serial.printf("Files open but failed to write in active.bin.\r\n"); break;
                    case 2:  Serial.printf("Files open but failed to write in calendarHeader.bin.\r\n"); break;
                    case 4:  Serial.printf("Failed to open active.bin.\r\n"); break;
                    case 8:  Serial.printf("Failed to open calendarHeader.bin.\r\n"); break;
                    case 12: Serial.printf("Failed to open both files.\r\n"); break;
                    default: Serial.printf("Unrecognised return value. Function fail.\r\n"); break;
                }
            }
            
            // SET THE RTC TIME because RTC module doesn't have cell (Breadboard test calibration)
            rtc.setTime(0, 30, 0, 21, 5, 26, 4); 
            /* End of dummy code */
            
            // RECALCULATE EPOCH (Fetch the new 2026 time we just injected)
            // rtc.getTime(t_hour, t_min, t_sec, t_date, t_month, t_year);
            rtc.getTime(currentTime);
            // Serial.printf("[DEBUG TIME] RTC Raw Output: %02d/%02d/%d %02d:%02d:%02d\n", 
            //       t_date, t_month, t_year, t_hour, t_min, t_sec);
            Serial.printf("[DEBUG TIME] RTC Raw Output: %02d/%02d/%d %02d:%02d:%02d\n", 
                  currentTime.date, currentTime.month, currentTime.year, currentTime.hour, currentTime.min, currentTime.sec);

            currentEpoch = convertDate2Epoch(&currentTime);
            
            // 5. REBUILD QUEUE & ARM ALARM
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            alarmManager.programNextAlarm(&rtc);
            Serial.printf("Total Events loaded: %d\n", storageManager.getTotalEvents());
            currentState = new HomeState();
            break;
        case 2:
            Serial.println("=== WOKE UP FROM RTC ALARM ===");
            rtc.getTime(currentTime);
            currentEpoch = convertDate2Epoch(&currentTime);
            alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            currentState = new AlarmState();
            break;
        case 4:
            Serial.println("=== WOKE UP FROM OK BUTTON ===");
            currentState = new HomeState();
            break;
    }
}

uint8_t SystemCtrl::read_buttons() {
    gpio_hold_dis((gpio_num_t)OK_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)CANCEL_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)UP_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)DOWN_BUTTON_PIN);
    gpio_hold_dis((gpio_num_t)WIFI_BUTTON_PIN);
    if(digitalRead(OK_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(OK_BUTTON_PIN) == LOW) {
            return 1;
        }
    }
    else if(digitalRead(CANCEL_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(CANCEL_BUTTON_PIN) == LOW) {
            return 2;
        }
    }
    else if(digitalRead(UP_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(UP_BUTTON_PIN) == LOW) {
            return 3;
        }
    }
    else if(digitalRead(DOWN_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(DOWN_BUTTON_PIN) == LOW) {
            return 4;
        }
    }
    else if(digitalRead(WIFI_BUTTON_PIN) == LOW) {
        delay(50);
        if(digitalRead(WIFI_BUTTON_PIN) == LOW) {
            return 5;
        }
    }
    else {
        return 0;
    }
}

void SystemCtrl::system_loop() {
    currentState->onEnter();
    uint32_t loopStart = millis();
    while(millis() - loopStart < screenTimeOut) {
        uint8_t buttonPressed = this->read_buttons();
        currentState->onProgress();
        if(buttonPressed != 0) {
            while(this->read_buttons() == buttonPressed) { 
                delay(10); 
                // Keep calling onProgress so animations don't freeze while holding a button
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
    // --- 6. GO TO SLEEP ---
    Serial.println("Entering Deep Sleep (RTC PIN ONLY)...");
    uint64_t wake_mask = (1ULL << RTC_PIN) | (1ULL << OK_BUTTON_PIN);
    esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    // 2. FORCE the C6 to keep the pullup alive during deep sleep
    gpio_hold_en((gpio_num_t)RTC_PIN);
    gpio_hold_en((gpio_num_t)OK_BUTTON_PIN);
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);
    esp_deep_sleep_start();
}

#endif