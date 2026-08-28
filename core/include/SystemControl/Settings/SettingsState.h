#pragma once
#ifndef SETTINGSSTATE_H
#define SETTINGSSTATE_H

#include "AppState.h"

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

#endif