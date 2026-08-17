#pragma once
#ifndef SYSCON_H
#define SYSCON_H

#include "RTC_DS3231.h"
#include "DataModels.h"
#include "StorageManager.h"
#include "TimeEngine.h"
// #include "AlarmManager.h"
#include "driver/gpio.h"
#include "SSD1306.h"
#include "Serial.h"

#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

// --- RTC INTERRUPT ---
#define RTC_PIN           GPIO_NUM_2   // Safe: Top Right header

// --- UI BUTTONS ---
#define OK_BUTTON_PIN     GPIO_NUM_11  // Safe: Bottom Right header
#define CANCEL_BUTTON_PIN GPIO_NUM_12  // Safe: Bottom Right header
#define UP_BUTTON_PIN     GPIO_NUM_13  // Safe: Bottom Right header
#define DOWN_BUTTON_PIN   GPIO_NUM_10  // Safe: Bottom Left header
#define WIFI_BUTTON_PIN   GPIO_NUM_9  // Safe: Bottom Left header

// --- ACTUATORS ---
#define MOTOR_PIN         GPIO_NUM_21  // Safe: Middle Left header

RTC_DATA_ATTR uint32_t lastAlarmEpoch = 0;

// 1. Declare the inter-core communication queue globally so both tasks can see the pipe
extern QueueHandle_t adc_data_queue;
extern volatile bool is_recording_gesture;

struct AdcFrame {
    int32_t channels[8];
};

// Helper function to make the Serial Monitor act like a refreshing screen
void clearConsole() {
    // Serial.print("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"); 
    ESP_LOGI("","\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r");
    // Serial.print("\033[2J\033[H"); 
}

struct AppContext {
    // RTC_DS3231* rtc;
    // AlarmManager* alarm_manager;
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

class HomeState;
class CalendarHomeState;
class AlarmState;
class WifiState;
class EventListState;
class EventDetailState;
class Settings;
class GestureState;

class HomeState : public AppState {
private:
    uint8_t cursorIndex;
    const Time* eventTime_pointer;
    char apps[2][10] = {"Home", "Settings"};
    uint8_t appCount = sizeof(apps)/sizeof(apps[0]);
public:
    HomeState(AppContext* app_context, uint8_t cursorIndex) {
        this->app_context = app_context;
        this->cursorIndex = cursorIndex;
    }
    void onEnter() override {
        clearConsole();
        uint8_t displayedAppLimit = 5;
        
        for(uint8_t i = 0; (i < displayedAppLimit) && (i < sizeof(apps)/sizeof(apps[0])); i++) {
            // 1. Declare and initialize the buffer INSIDE the loop so it resets every iteration
            char app_senc[16] = ""; 
            
            if(i == cursorIndex) {
                // 2. Use strcpy for the first string to guarantee a clean slate
                strcpy(app_senc, "> ");
            }
            else {
                strcpy(app_senc, "  ");
            }
            
            // 3. Now it is safe to use strcat
            strcat(app_senc, (const char*)apps[i]);
            
            this->app_context->display->drawStringRight(10 + (i)*10, app_senc, 1);
        }
    }
    void onProgress() override {
        this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override {
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

class CalendarHomeState : public AppState {
private:
    uint8_t displayedEventIndex;
    const Time* eventTime_pointer;
public:
    CalendarHomeState(AppContext* app_context, uint8_t display_event_index) {
        this->app_context = app_context;
        this->displayedEventIndex = display_event_index;
        // FIX 3: No local copies of time or counts! We fetch dynamically on load.
    }

    void onEnter() override {
        clearConsole();
        
        // Fetch fresh data directly from the system context
        // Time* t = app_context->currentTime;
        // uint8_t alarmCount = app_context->alarm_manager->getAlarmCount();
        // const AlarmNode* alarmQueue = app_context->alarm_manager->getAlarmQueue();
        const Event* eventsArray = app_context->storage_manager->getEventsArray();
        uint16_t totalEvents = app_context->storage_manager->getTotalEvents();
        // uint8_t missedCount = app_context->alarm_manager->getMissedCount();

        // Serial.println("=========================================");
        // Serial.println("             [ HOME SCREEN ]             ");
        // Serial.println("=========================================");
        // Serial.printf(" TIME: %02d:%02d:%02d\r\n", t->hour, t->min, t->sec);
        // Serial.printf(" DATE: %02d/%02d/%d\r\n", t->date, t->month, t->year);
        // Serial.println("-----------------------------------------");
        
        // if (alarmCount > displayedEventIndex) {
        //     uint16_t currentID = alarmQueue[displayedEventIndex].eventID;
        //     for(uint16_t i=0; i < totalEvents; i++) {
        //         if (eventsArray[i].id == currentID) {
        //             // Serial.printf(" UPCOMING: %s\r\n", eventsArray[i].name);
        //             eventTime_pointer = &eventsArray[i].eventTime;
        //             this->app_context->display->drawStringCentered(47, eventsArray[i].name, 1);
        //             break;
        //         }
        //         else {
        //             eventTime_pointer = nullptr;
        //         }
        //     }
        // } else {
        //     // Serial.println(" UPCOMING: None");
        //     eventTime_pointer = nullptr;
        //     // this->app_context->display->drawStringCentered(47, "None", 1);
        // }
        
        // Serial.println("-----------------------------------------");
        // if (missedCount > 0) {
        //     // Serial.printf(" *** %d MISSED EVENT(S)! ***\r\n", missedCount);
        // } else {
        //     // Serial.println(" No Missed Events.");
        // }
        // Serial.println("=========================================\r\n");
        // char missed_senc[13];
        // char battery_senc[5];
        char missed_senc[13] = "MISSED: 0";
        char battery_senc[5] = "100%";
        // sprintf(missed_senc, "MISSED: %d", missedCount);
        // sprintf(battery_senc, "%d%%", 100);
        this->app_context->display->drawStringRight(1, missed_senc, 1);
        this->app_context->display->drawString(1, 1, battery_senc, 1);
        this->app_context->display->drawStringCentered(37, "UPCOMING", 1);
        // this->app_context->display->drawStringCentered(48, "meeting with sir", 1); // WIP: to be removed
    }

    void onProgress() override {
        Time* t = app_context->currentTime;
        // char time_senc[9];
        char time_senc[9] = "12:00:00"; 
        // sprintf(time_senc, "%02d:%02d:%02d", t->hour, t->min, t->sec);
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
            // float progressBarLength = (float)(convertDate2Epoch(eventTime_pointer) - convertDate2Epoch(t))*127.0/(convertDate2Epoch(eventTime_pointer) - *(app_context->lastAlarmEpoch));
            // this->app_context->display->drawLine(1, 62, (uint8_t)progressBarLength, 62, 1);
            // // Serial.println((uint8_t)progressBarLength);
        }
        this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override {
        // Serial.println("Clear screen");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};
class Settings : public AppState {
private:

    uint8_t cursorIndex = 0;
    static constexpr uint8_t TOTAL_ITEMS =2;

    const char* menuItems[TOTAL_ITEMS]{
        "Bluetooth",
        "Gestures"
    };
public:

    Settings(AppContext* app_context){
        this-> app_context = app_context;
        this -> cursorIndex =0;

    }

    void onEnter() override {
        clearConsole();
        
        for (uint8_t i = 0; i <TOTAL_ITEMS; i++){
            char lineBuffer[24] ="";

            if(i == this->cursorIndex){
                strcpy(lineBuffer,"> ");
            }
            else {
                strcpy(lineBuffer,"  ");
            }

            strcat(lineBuffer, this->menuItems[i]);

            int16_t y_pos = 20 + ( i * 30);

            if (this->app_context && this->app_context->display ){
                this->app_context-> display->drawStringRight(y_pos,lineBuffer,1);
                
                
            }
        }

    }
    void onProgress() override {}
    AppState* handleInput(uint8_t buttonPressed) override;    
    
    void onExit() override {
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }


};

class AlarmState : public AppState {
private:
    uint8_t debug_print_limit;
    AnimationHeader animHead;
    File animFile;
    uint32_t lastFrameTime;
    uint8_t currentFrameIndex;
    uint8_t* animFrame;
public:
    AlarmState(AppContext* app_context) : animFile(nullptr) {
        this->app_context = app_context;
        debug_print_limit = 1;
        lastFrameTime = esp_timer_get_time() / 1000;
        animFile = LittleFS.open("/AlarmStateAnimation.bin", "r");
        if(!animFile) {
            // Serial.println("unable to open UI file.");
        }
        currentFrameIndex = 0;
        animFrame = nullptr;
    }

    void onEnter() override {
        clearConsole();
        // Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        // Serial.println("!!             ALARM RINGING           !!");
        // Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        // Serial.println(" (Motor Vibrating...)");
        analogWrite(MOTOR_PIN, 128);
        // lastAlarmEpoch = convertDate2Epoch(this->app_context->currentTime);
        if(animFile) {
            animFile.read((uint8_t*)&animHead, sizeof(AnimationHeader));
            animFrame = new uint8_t[(animHead.width * animHead.height) / 8];
            // Serial.printf("[SYSTEM] created animFrame with %d bytes.\r\n", (animHead.width * animHead.height) / 8);
            animFile.read((uint8_t*)animFrame, (animHead.width * animHead.height) / 8);
        }
    }

    void onProgress() override {
        if(debug_print_limit) {
            // Serial.println("Update frame for animation.");
            debug_print_limit = 0;
        }
        if(!animFile || animFrame == nullptr) { 
            // Serial.println("file not opened or nullptr.");
            return;
        }
        uint32_t currentFrameTime = esp_timer_get_time() / 1000;
        if(currentFrameTime - lastFrameTime > animHead.frame_delay) {
            lastFrameTime = currentFrameTime;
            currentFrameIndex = (currentFrameIndex + 1) % animHead.frame_count;
            animFile.seek(sizeof(AnimationHeader) + currentFrameIndex * ((animHead.width * animHead.height) / 8), SEEK_SET);
            animFile.read((uint8_t*)animFrame, ((animHead.width * animHead.height) / 8));
            // Serial.printf("[SYSTEM] frame changed to %d\r\n", currentFrameIndex);
        }
        else {
            this->app_context->display->clearBuffer();
            this->app_context->display->drawFrame(1, 1, animFrame, (animHead.width), animHead.height);
            this->app_context->display->updateDisplay();
            // Serial.printf("[SYSTEM] drawing frame%d\r\n", currentFrameIndex);
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
        // Serial.println("=========================================");
        // Serial.println("             [ WIFI MODULE ]             ");
        // Serial.println("=========================================");
        // Serial.println(" Status: Turning On...");
        // Serial.println(" SSID: ESP32-Watch");
        // Serial.println(" IP: 192.168.4.1");
        // Serial.println("=========================================\r\n");
    }

    void onProgress() override { return; }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override { 
        // Serial.println("[SYSTEM] Turning off WiFi Module...");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};
class GestureState: public AppState {
private:
    char labels[6][12] = {
        "label1",
        "label2",
        "label3",
        "label4",
        "label5",
        "USB Sync"
    };
    uint8_t cursor_index;
public:
    GestureState(AppContext* app_context) {
        this->app_context = app_context;
        this->cursor_index = 0;
    }
    void onEnter() override {
        uint8_t displayLabelLimit = 5;
        uint8_t loopCount = sizeof(labels)/sizeof(labels[0]);
        for(uint8_t i = cursor_index + 1; (loopCount > 0) && (displayLabelLimit > 0); loopCount--) {
            if(i-1 == cursor_index) {
                this->app_context->display->drawString(1, 10 + (i - 1 - cursor_index)*10, ">", 1);
                this->app_context->display->drawString(7, 10 + (i - 1 - cursor_index)*10, labels[i], 1);
            }
            else {
                if((int)i - 1 - (int)cursor_index < 0) {
                    // Added the '10 +' base offset
                    this->app_context->display->drawString(7, 10 + (sizeof(labels)/sizeof(labels[0]) + i - 1 - cursor_index)*10, labels[i], 1);
                } else {
                    // Added the '10 +' base offset
                    this->app_context->display->drawString(7, 10 + (i - 1 - cursor_index)*10, labels[i], 1);
                }
            }
            i = i + 1;
            if(i == sizeof(labels)/sizeof(labels[0]) + 1) {
                i = 1;
            }
            displayLabelLimit -= 1;
        }
    }
    void onProgress() override {}
    void onExit() override {
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;
};

class GestureRecordState: public AppState {
private:
    File recordFile;
    uint8_t* pageBuffer;        // 4KB Raw Byte Buffer in PSRAM
    uint16_t bufferOffset;      // Tracks byte position in the 4KB buffer
    uint32_t recordStartTime;   
    char currentLabel[10];
    
    bool isFirstFrame;          // Delta compression flag
    AdcFrame previousFrame;     // Holds S_{n-1} for delta calculation
public:
    GestureRecordState(AppContext* app_context, const char* label) {
        this->app_context = app_context;
        strcpy(this->currentLabel, label);
        this->bufferOffset = 0;
        this->pageBuffer = nullptr;
        this->isFirstFrame = true;
    }
    void onEnter() override {
        clearConsole();
        
        // 1. Allocate a flat 4,096-byte page buffer in PSRAM
        pageBuffer = (uint8_t*) heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
        
        if (pageBuffer == nullptr) {
            ESP_LOGE("GESTURE", "PSRAM allocation failed!");
            return;
        }

        // 2. Open LittleFS File
        char filename[32];
        // sprintf(filename, "/%s_%lu.bin", currentLabel, convertDate2Epoch(this->app_context->currentTime));
        sprintf(filename, "/%s_%lu.bin", currentLabel, (uint64_t)esp_timer_get_time() / 1000000);
        recordFile = LittleFS.open(filename, "w");

        // 3. Update OLED
        this->app_context->display->clearBuffer();
        this->app_context->display->drawStringCentered(20, "RECORDING...", 1);
        this->app_context->display->drawStringCentered(40, currentLabel, 1);
        this->app_context->display->updateDisplay();

        // 4. Start Capture
        recordStartTime = esp_timer_get_time() / 1000;
        is_recording_gesture = true; 
    }
    void onProgress() override {
        if(!recordFile || pageBuffer == nullptr) return;

        AdcFrame incomingFrame;

        // Pull from Queue
        if (xQueueReceive(adc_data_queue, &incomingFrame, 0) == pdPASS) {
            
            if (isFirstFrame) {
                // WRITE ABSOLUTE (32 Bytes)
                // Check if buffer has room (it always will on first frame)
                memcpy(&pageBuffer[bufferOffset], &incomingFrame, sizeof(AdcFrame));
                bufferOffset += sizeof(AdcFrame); // 32
                
                previousFrame = incomingFrame;
                isFirstFrame = false;
            } 
            else {
                // WRITE DELTA (16 Bytes)
                int16_t deltas[8];
                for(uint8_t i = 0; i < 8; i++) {
                    // Calculate S_n - S_{n-1}
                    deltas[i] = (int16_t)(incomingFrame.channels[i] - previousFrame.channels[i]);
                }

                // Check if we have 16 bytes of space left in the 4KB buffer
                if (bufferOffset + sizeof(deltas) > 4096) {
                    recordFile.write(pageBuffer, bufferOffset);
                    bufferOffset = 0; // Reset
                }

                memcpy(&pageBuffer[bufferOffset], deltas, sizeof(deltas));
                bufferOffset += sizeof(deltas); // 16
                
                previousFrame = incomingFrame;
            }
        }

        // 3-Second Timeout Check
        if ((esp_timer_get_time() / 1000) - recordStartTime >= 3000) {
            this->handleInput(2); // Simulate CANCEL to exit
        }
    }
    void onExit() override {
        is_recording_gesture = false;

        // Flush remaining bytes
        if (recordFile && bufferOffset > 0) {
            recordFile.write(pageBuffer, bufferOffset);
        }

        if(recordFile) recordFile.close();
        if(pageBuffer != nullptr) heap_caps_free(pageBuffer);
        xQueueReset(adc_data_queue);
        
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;
};

class GestureSyncState: public AppState {
private:
    bool syncComplete;
public:
    GestureSyncState(AppContext* app_context) {
        this->app_context = app_context;
        this->syncComplete = false;
    }

    void onEnter() override {
        clearConsole();
        
        this->app_context->display->clearBuffer();
        this->app_context->display->drawStringCentered(20, "USB SYNC", 1);
        this->app_context->display->drawStringCentered(40, "Waiting for PC...", 1);
        this->app_context->display->updateDisplay();

        // 1. Shift UART to High Speed (2 Mbps)
        Serial.flush();
        Serial.end();
        Serial.begin(2000000); 
    }

    void onProgress() override {
        if (syncComplete) return;

        // 2. Placeholder for PC Handshake Protocol
        if (Serial.available()) {
            uint8_t cmd = Serial.read();
            if (cmd == 0xAA) {
                // Send metadata, read LittleFS files, and blast 4KB chunks here.
                // ...
                // Once finished:
                syncComplete = true;
                this->app_context->display->clearBuffer();
                this->app_context->display->drawStringCentered(30, "SYNC COMPLETE", 1);
                this->app_context->display->updateDisplay();
            }
        }
    }

    AppState* handleInput(uint8_t buttonPressed) override {
        // Press CANCEL to abort sync and return to menu
        if(buttonPressed == 2) { 
            return new GestureState(this->app_context); 
        }
        return this;
    }

    void onExit() override {
        // 3. Restore Standard UART Speed for OS logs
        Serial.flush();
        Serial.end();
        Serial.begin(115200); 

        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
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
        // Serial.println("=========================================");
        // Serial.println("           [ MISSED EVENTS ]             ");
        // Serial.println("=========================================");
        this->app_context->display->clearBuffer();
        this->app_context->display->drawStringCentered(1, "MISSED ALARMS", 1);
        
        // Fetch dynamically!
        // const Event* eventsArray = app_context->storage_manager->getEventsArray();
        // uint16_t totalEvents = app_context->storage_manager->getTotalEvents();
        // const AlarmNode* missedQueue = app_context->alarm_manager->getMissedQueue();
        // uint8_t missedCount = app_context->alarm_manager->getMissedCount();
        
        // if (missedCount == 0) {
        //     // Serial.println("  (List is empty)");
        //     this->app_context->display->drawStringCentered(23, "No missed events.", 1);
        // } else {
        //     uint8_t loopCount = missedCount;
        //     uint8_t displayLimit = 5;
        //     for(uint8_t i = displayedEventIndex + 1; (loopCount > 0) && (displayLimit > 0); loopCount--) {
        //         uint16_t j;
        //         for(j = 0; j < totalEvents; j++) {
        //             if(missedQueue[i-1].eventID == eventsArray[j].id) break;
        //         }
        //         if(j < totalEvents) {
        //             if (i-1 == displayedEventIndex) {
        //                 // Serial.printf(" -> %s (%02d:%02d)\r\n", eventsArray[j].name, eventsArray[j].eventTime.hour, eventsArray[j].eventTime.min);
        //                 this->app_context->display->drawString(1, 10 + (i - 1 - displayedEventIndex)*10, ">", 1);
        //                 this->app_context->display->drawString(7, 10 + (i - 1 - displayedEventIndex)*10, eventsArray[j].name, 1);
        //             } else {
        //                 // Serial.printf("    %s (%02d:%02d)\r\n", eventsArray[j].name, eventsArray[j].eventTime.hour, eventsArray[j].eventTime.min);
        //                 // Explicitly cast to (int) to safely perform negative checks on unsigned variables
        //                 if((int)i - 1 - (int)displayedEventIndex < 0) {
        //                     // Added the '10 +' base offset
        //                     this->app_context->display->drawString(7, 10 + (missedCount + i - 1 - displayedEventIndex)*10, eventsArray[j].name, 1);
        //                 } else {
        //                     // Added the '10 +' base offset
        //                     this->app_context->display->drawString(7, 10 + (i - 1 - displayedEventIndex)*10, eventsArray[j].name, 1);
        //                 }
        //             }
        //         }
        //         i = i + 1;
        //         if(i == missedCount + 1) {
        //             i = 1;
        //         }
        //         displayLimit -= 1;
        //     }
        // }
        this->app_context->display->updateDisplay();
        // Serial.println("=========================================\r\n");
    }

    void onProgress() override { 
        return;
    }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override { 
        // Serial.println("Clear screen");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
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
        this->app_context->display->clearBuffer();
        // Serial.println("=========================================");
        // Serial.println("           [ EVENT DETAILS ]             ");
        // Serial.println("=========================================");
        this->app_context->display->drawStringCentered(1, "EVENT DETAILS", 1);
        if(event_pointer != nullptr) {
            // Serial.printf(" Name:    %s\r\n", event_pointer->name);
            this->app_context->display->drawStringCentered(20, event_pointer->name, 1);
            // Serial.printf(" Time:    %02d:%02d:%02d\r\n", event_pointer->eventTime.hour, event_pointer->eventTime.min, event_pointer->eventTime.sec);
            char time_string[12];
            // sprintf(time_string, "%02d:%02d:%02d", event_pointer->eventTime.hour, event_pointer->eventTime.min, event_pointer->eventTime.sec);
            this->app_context->display->drawString(1, 30, time_string, 1);
            // Serial.printf(" Date:    %02d/%02d/%d\r\n", event_pointer->eventTime.date, event_pointer->eventTime.month, event_pointer->eventTime.year);
            // sprintf(time_string, "%02d/%02d/%02d", event_pointer->eventTime.date, event_pointer->eventTime.month, event_pointer->eventTime.year);
            this->app_context->display->drawStringRight(30, time_string, 1);
            // Serial.println("-----------------------------------------");
            // Serial.printf(" Details: %s\r\n", event_pointer->details);
            this->app_context->display->drawStringWrapped(1, 40, event_pointer->details, 1);
        } else {
            // Serial.println(" Error: Event Not Found.");
        }
        // Serial.println("=========================================\r\n");
        this->app_context->display->updateDisplay();
    }

    void onProgress() override { return; }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override { 
        // Serial.println("Clear screen");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

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
            cursorIndex = sizeof(this->appCount) - 1;
        }
        else {
            cursorIndex--;
        }
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 4) {
        cursorIndex = (cursorIndex + 1) % appCount;
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
        if(cursorIndex > 0){
            cursorIndex--;
            this->onEnter();
        }
    }
    else if(buttonPressed == 4){//DOWN Button
        if(cursorIndex < TOTAL_ITEMS - 1){
            cursorIndex++;
            this->onEnter();
        }
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
        // if(this->app_context->alarm_managetAlarmCount() > displayedEventIndex) {
        //     return new EventDetailState(this-r->ge>app_context, this->app_context->alarm_manager->getAlarmQueue()[displayedEventIndex].eventID);
        // }
        return this;
    }
    else if(buttonPressed == 2) { return new HomeState(this->app_context, 0); }
    else if(buttonPressed == 3) {
        // uint8_t alarmCount = this->app_context->alarm_manager->getAlarmCount();
        // if(alarmCount > 1) {
        //     // Replaced '% 2' with '% alarmCount' so you can cycle through ALL upcoming alarms
        //     displayedEventIndex = (displayedEventIndex + 1) % 2;
        // }
        // this->onEnter();
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
    // this->app_context->rtc->getTime(*(this->app_context->currentTime));
    // uint32_t currentEpoch = convertDate2Epoch(this->app_context->currentTime);
    // this->app_context->alarm_manager->rebuildQueue(this->app_context->storage_manager->getEventsArray(), this->app_context->storage_manager->getTotalEvents(), currentEpoch);
    // this->app_context->alarm_manager->programNextAlarm(this->app_context->rtc);
    if(animFile) {
        animFile.close();
        delete[] animFrame;
        animFrame = nullptr;
        // Serial.println("freed allocated bytes.");
    }
}

inline AppState* WifiState::handleInput(uint8_t buttonPressed) {
    if(buttonPressed == 1) { return this; }
    else if(buttonPressed == 2) { return new CalendarHomeState(this->app_context, 0); }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

inline AppState* EventListState::handleInput(uint8_t buttonPressed) {
    // uint8_t missed_event_count = this->app_context->alarm_manager->getMissedCount();
    
    if(buttonPressed == 1) {
        // if(missed_event_count > displayedEventIndex) {
        //     return new EventDetailState(this->app_context, (this->app_context->alarm_manager->getMissedQueue()[displayedEventIndex]).eventID);
        // }
        return this;
    }
    else if(buttonPressed == 2) { return new CalendarHomeState(this->app_context, 0); }
    else if(buttonPressed == 3) {
        // if(missed_event_count != 0) {
        //     displayedEventIndex = (displayedEventIndex + 1) % missed_event_count;
        // }
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 4) {
        // if(missed_event_count != 0) {
        //     displayedEventIndex = (displayedEventIndex + missed_event_count - 1) % missed_event_count;
        // }
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

        // Time event_time = event_pointer->eventTime;
        // uint8_t newDisplayedEventIndex = 0; 
        
        // if(compareDateTime(&event_time, this->app_context->currentTime) >= 0) {
        //     const AlarmNode* alarmQueue = this->app_context->alarm_manager->getAlarmQueue();
        //     uint8_t alarmQueueCount = this->app_context->alarm_manager->getAlarmCount();
        //     for(uint8_t i = 0; i < alarmQueueCount; i++) {
        //         if(alarmQueue[i].eventID == displayedEventID) {
        //             newDisplayedEventIndex = i;
        //             break;
        //         }
        //     }
        //     return new CalendarHomeState(this->app_context, newDisplayedEventIndex);
        // }
        // else {
        //     const AlarmNode* missedQueue = this->app_context->alarm_manager->getMissedQueue();
        //     uint8_t missedQueueCount = this->app_context->alarm_manager->getMissedCount();
        //     for(uint8_t i = 0; i < missedQueueCount; i++) {
        //         if(missedQueue[i].eventID == displayedEventID) {
        //             newDisplayedEventIndex = i;
        //             break;
        //         }
        //     }
        //     return new EventListState(this->app_context, newDisplayedEventIndex);
        // }
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
        if (cursor_index == 5) {
            return new GestureSyncState(this->app_context);
        } 
        // 2. Otherwise, start recording the selected gesture
        else {
            return new GestureRecordState(this->app_context, labels[cursor_index]); 
        }
    }
    else if(buttonPressed == 2) { return new Settings(this->app_context); }
    else if(buttonPressed == 3) { 
        uint8_t label_count = sizeof(labels)/sizeof(labels[0]);
        cursor_index = (cursor_index + 1) % label_count;
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 4) {
        uint8_t label_count = sizeof(labels)/sizeof(labels[0]);
        cursor_index = (cursor_index + label_count - 1) % label_count;
        this->onEnter();
        return this;
    }
    else if(buttonPressed == 5) { return this; }
    else if(buttonPressed == 6) { return new AlarmState(this->app_context); }
    return this;
}

AppState* GestureRecordState::handleInput(uint8_t buttonPressed) {
    // Button 2 (CANCEL) or the 3-second timeout will trigger the exit
    if(buttonPressed == 2) { 
        return new GestureState(this->app_context); 
    }
    return this;
}

// ================= SYSTEM CONTROLLER =================

class SystemCtrl {
    private:
    Time currentTime;
    // RTC_DS3231 rtc;
    StorageManager storageManager;
    // AlarmManager alarmManager;
    esp_sleep_wakeup_cause_t wakeup_reason;
    uint32_t wakeup_mask;
    uint8_t boot_state;
    AppState* currentState;
    uint32_t screenTimeOut;
    AppContext appContext;
    uint32_t loopStart;
    SSD1306 display;

    public:
    SystemCtrl(uint32_t timeout);
    void init();
    void boot_handler();
    void system_loop();
    uint8_t read_buttons();
    void shutdown_handler();
};

// SystemCtrl::SystemCtrl(uint32_t timeout) : rtc(15, 14), screenTimeOut{timeout}, display(7, 15, 6, 5, 4) {
SystemCtrl::SystemCtrl(uint32_t timeout) : screenTimeOut{timeout}, display(7, 15, 6, 5, 4) {
    pinMode(OK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);
    pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
    pinMode(WIFI_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RTC_PIN, INPUT_PULLUP);
    currentState = nullptr;
    // appContext.alarm_manager = &alarmManager;
    // appContext.rtc = &rtc;
    appContext.storage_manager = &storageManager;
    appContext.display = &display;
    appContext.lastAlarmEpoch = &lastAlarmEpoch;
}

void SystemCtrl::init() {
    // rtc.begin();
    // rtc.getTime(currentTime);
    appContext.currentTime = &currentTime;
    display.ssd1306_init();
    display.clearBuffer();
    // Serial.println("\r\n[SYSTEM] Booting...");
            
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
    // uint32_t currentEpoch;

    if (currentState != nullptr) {
        delete currentState;
        currentState = nullptr;
    }

    switch(boot_state) {
        case 1:
            // Serial.println("[SYSTEM] Cold Boot (Battery Connected)");
            // rtc.getTime(currentTime);
            // currentEpoch = convertDate2Epoch(&currentTime);
            // alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            // alarmManager.programNextAlarm(&rtc);
            // Serial.printf("Total Events loaded: %d\r\n", storageManager.getTotalEvents());
            currentState = new HomeState(&appContext, 0);
            // lastAlarmEpoch = convertDate2Epoch(&currentTime);
            break;
        case 2:
            // Serial.println("[SYSTEM] Woke up from RTC ALARM");
            // rtc.getTime(currentTime);
            // currentEpoch = convertDate2Epoch(&currentTime);
            // alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            // alarmManager.programNextAlarm(&rtc);
            currentState = new AlarmState(&appContext);
            break;
        case 4:
            // Serial.println("[SYSTEM] Woke up from OK BUTTON");
            // rtc.getTime(currentTime);
            // currentEpoch = convertDate2Epoch(&currentTime);
            
            // FIX 1: Rebuild the queue here so amnesia is cured!
            // alarmManager.rebuildQueue(storageManager.getEventsArray(), storageManager.getTotalEvents(), currentEpoch);
            // alarmManager.programNextAlarm(&rtc);
            
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
    if(digitalRead(OK_BUTTON_PIN) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if(digitalRead(OK_BUTTON_PIN) == LOW) { return 1; }
    }
    else if(digitalRead(CANCEL_BUTTON_PIN) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if(digitalRead(CANCEL_BUTTON_PIN) == LOW) { return 2; }
    }
    else if(digitalRead(UP_BUTTON_PIN) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if(digitalRead(UP_BUTTON_PIN) == LOW) { return 3; }
    }
    else if(digitalRead(DOWN_BUTTON_PIN) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if(digitalRead(DOWN_BUTTON_PIN) == LOW) { return 4; }
    }
    else if(digitalRead(WIFI_BUTTON_PIN) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if(digitalRead(WIFI_BUTTON_PIN) == LOW) { return 5; }
    }
    else if(digitalRead(RTC_PIN) == LOW) {
        // this->appContext.rtc->clearAlarm1();
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
    loopStart = esp_timer_get_time() / 1000;
    uint32_t lastRefresh = esp_timer_get_time() / 1000;
    while(esp_timer_get_time() / 1000 - loopStart < screenTimeOut) {
        // appContext.rtc->getTime(currentTime);
        uint8_t buttonPressed = this->read_buttons();
        // ADD THIS: Rate Limit the UI rendering to ~30Hz (33ms)
        uint32_t now = esp_timer_get_time() / 1000;
        if (now - lastRefresh >= 33) {
            currentState->onProgress();
            lastRefresh = now;
        }
        if(buttonPressed != 0) {
            // const char* btnNames[] = {"NONE", "OK", "CANCEL", "UP", "DOWN", "WIFI", "RTC_ALARM"};
            // Serial.printf("[INPUT] Button Registered: %s (%d)\r\n", btnNames[buttonPressed], buttonPressed);
            
            while(this->read_buttons() == buttonPressed) { 
                vTaskDelay(pdMS_TO_TICKS(10));
                if ((esp_timer_get_time() / 1000) - lastRefresh >= 33) {
                    currentState->onProgress(); 
                    lastRefresh = esp_timer_get_time() / 1000;
                }
            }
            AppState* nextState = currentState->handleInput(buttonPressed);
            if(nextState != currentState) {
                currentState->onExit();
                delete currentState;
                currentState = nextState;
                currentState->onEnter();
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
    // appContext.display->clearBuffer();
    // appContext.display->updateDisplay();
    appContext.display->ssd1306_shutdown();
    // Serial.println("\r\n[SYSTEM] Timeout reached. Entering Deep Sleep...");
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
    // Serial.println("\r\n[SYSTEM] Woke up from Light Sleep!");

    gpio_hold_dis((gpio_num_t)RTC_PIN);
    gpio_hold_dis((gpio_num_t)OK_BUTTON_PIN);

    // 1. Re-evaluate the wakeup reason
    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    
    boot_state = 0; // Clear old state
    boot_state |= ((wakeup_pin_mask & (1ULL << RTC_PIN)) > 0) << 1;         
    boot_state |= ((wakeup_pin_mask & (1ULL << OK_BUTTON_PIN)) > 0) << 2;  

    // 2. Call your safe boot_handler!
    this->boot_handler();

    // 3. Power up the screen
    appContext.display->ssd1306_init();
    appContext.display->clearBuffer();
    appContext.display->updateDisplay();

    loopStart = esp_timer_get_time() / 1000;
}

#endif