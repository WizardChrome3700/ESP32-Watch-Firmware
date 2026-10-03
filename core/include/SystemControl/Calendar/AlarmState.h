#pragma once
#ifndef ALARMSTATE_H
#define ALARMSTATE_H

#include "AppState.h"

/**
 * @class AlarmState
 * @brief AppState derivative used to handle event alarms from RTC.
 * @details It is used to handle running animations stored in memory on the display.
 */
class AlarmState : public AppState {
private:
    uint8_t debug_print_limit;
    AnimationHeader animHead;
    File animFile;
    uint32_t lastFrameTime;
    uint8_t currentFrameIndex;
    uint8_t* animFrame;
public:
    /**
     * @brief
     * It initialises the file pointer that points to the memory storing the animation frames.
     * @details
     * - It passes obtains the AppContext pointer and initializes the frame counter and frame timers.
     * - It initialises the file pointer that points to the memory storing the animation frames.
     */
    AlarmState(AppContext* app_context) {
        this->app_context = app_context;
        debug_print_limit = 1;
        lastFrameTime = esp_timer_get_time() / 1000;
        animFile = LittleFS.open("/AlarmStateAnimation.bin", "r");
        if(!animFile) {
            Serial.println("unable to open UI file.");
        }
        currentFrameIndex = 0;
        animFrame = nullptr;
    }

    /**
     * @brief Called when transitioning into the alarm active state.
     * 
     * @details This lifecycle method handles the initial setup required when the alarm triggers:
     * - **UI Reset:** Clears both the serial console interface and the display buffer.
     * - **Hardware Notification:** Logs a prominent alert banner to the serial stream and activates the vibration motor at ~50% duty cycle (`128`).
     * - **Time Tracking:** Formats and caches the current time into `lastAlarmEpoch` for state logging.
     * - **Asset Allocation:** If a valid animation file (`animFile`) is available, it reads the header data and dynamic-allocates a pixel buffer (`animFrame`) to stream frames.
     * 
     * @note This function dynamic-allocates memory for `animFrame` which must be explicitly managed or freed upon exiting this state to prevent memory leaks.
     */
    void onEnter() override {
        clearConsole();
        this->app_context->display->clearBuffer();
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println("!!             ALARM RINGING           !!");
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println(" (Motor Vibrating...)");
        analogWrite(MOTOR_PIN, 128);
        lastAlarmEpoch = convertDate2Epoch(this->app_context->currentTime);
        if(animFile) {
            animFile.read((uint8_t*)&animHead, sizeof(AnimationHeader));
            animFrame = new uint8_t[(animHead.width * animHead.height) / 8];
            Serial.printf("[SYSTEM] created animFrame with %d bytes.\r\n", (animHead.width * animHead.height) / 8);
            animFile.read((uint8_t*)animFrame, (animHead.width * animHead.height) / 8);
        }
    }

    /**
     * @brief Called when the alarm state is being used.
     * @details
     * - it checks if the animation frames containing file has been opened.
     * - it updates the animation frames at a rate specified by the animation assets file.
     */
    void onProgress() override {
        if(debug_print_limit) {
            Serial.println("Update frame for animation.");
            debug_print_limit = 0;
        }
        if(!animFile || animFrame == nullptr) { 
            Serial.println("file not opened or nullptr.");
            return;
        }
        uint32_t currentFrameTime = esp_timer_get_time() / 1000;
        if(currentFrameTime - lastFrameTime > animHead.frame_delay) {
            lastFrameTime = currentFrameTime;
            currentFrameIndex = (currentFrameIndex + 1) % animHead.frame_count;
            animFile.seek(sizeof(AnimationHeader) + currentFrameIndex * ((animHead.width * animHead.height) / 8), SEEK_SET);
            animFile.read((uint8_t*)animFrame, ((animHead.width * animHead.height) / 8));
            Serial.printf("[SYSTEM] frame changed to %d\r\n", currentFrameIndex);
        }
        else {
            this->app_context->display->clearBuffer();
            this->app_context->display->drawFrame(1, 1, animFrame, (animHead.width), animHead.height);
            this->app_context->display->updateDisplay();
            Serial.printf("[SYSTEM] drawing frame%d\r\n", currentFrameIndex);
        }
    }

    AppState* handleInput(uint8_t buttonPressed) override;

    /**
     * @brief Called when the system transitions out of alarm state
     * @details
     * - it stops the motor from vibrating.
     * - it clears the display.
     * - it updates the alarm queue and programs RTC with the next alarm.
     * - it deletes the animFrame array pointer and frees it's heap memory.
     */
    void onExit() override {
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
};

#endif