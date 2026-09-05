#pragma once
#ifndef HOMESTATE_H
#define HOMESTATE_H

#include "AppState.h"

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
        // this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override {
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

#endif