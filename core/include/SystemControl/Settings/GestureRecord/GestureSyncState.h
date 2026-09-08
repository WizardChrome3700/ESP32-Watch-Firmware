#pragma once
#ifndef GESTURESYNCSTATE_H
#define GESTURESYNCSTATE_H

#include "AppState.h"

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

#endif