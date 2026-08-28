#pragma once
#ifndef GESTURESTATE_H
#define GESTURESTATE_H
#include "AppState.h"

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

#endif