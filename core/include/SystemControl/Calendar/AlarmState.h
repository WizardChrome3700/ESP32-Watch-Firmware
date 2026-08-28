#pragma once
#ifndef ALARMSTATE_H
#define ALARMSTATE_H

#include "AppState.h"

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
            Serial.println("unable to open UI file.");
        }
        currentFrameIndex = 0;
        animFrame = nullptr;
    }

    void onEnter() override {
        clearConsole();
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
    void onExit() override;
};

#endif