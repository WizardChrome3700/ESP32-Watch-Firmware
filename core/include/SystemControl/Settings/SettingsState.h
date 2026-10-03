#pragma once
#ifndef SETTINGSSTATE_H
#define SETTINGSSTATE_H

#include "AppState.h"

class Settings : public AppState {
private:

    uint8_t cursorIndex = 0;
    uint8_t appCount = 2;
    const char* menuItems[2]{
        "Bluetooth",
        "Gestures"
    };
public:

    Settings(AppContext* app_context){
        this-> app_context = app_context;
        this -> cursorIndex = 0;
    }

    void onEnter() override {
        clearConsole();
        uint8_t displayedAppLimit = 5;
        this->app_context->display->clearBuffer();

        Serial.println("=========================================");
        Serial.println("         [ CALENDAR HOME SCREEN ]        ");
        Serial.println("=========================================");
        Serial.printf(" TIME: %02d:%02d:%02d\r\n", app_context->currentTime->hour, app_context->currentTime->min, app_context->currentTime->sec);
        Serial.printf(" DATE: %02d/%02d/%d\r\n", app_context->currentTime->date, app_context->currentTime->month, app_context->currentTime->year);
        Serial.println("-----------------------------------------");

        uint8_t loopCount = appCount;
        for(uint8_t i = cursorIndex; (displayedAppLimit > 0) && (loopCount > 0); loopCount--) {
            char app_senc[24] = "";
            if(i == cursorIndex) {
                strcat(app_senc, "> ");
            }
            else {
                strcat(app_senc, "  ");
            }
            strcat(app_senc, menuItems[i]);
            if((int)i - (int)cursorIndex < 0) {
                this->app_context->display->drawString(1, 15 + (appCount + i - cursorIndex)*10, app_senc, 1);
            } else {
                this->app_context->display->drawString(1, 15 + (i - cursorIndex)*10, app_senc, 1);
            }
            Serial.printf("%d: %s\r\n", i, app_senc);
            i = i + 1;
            if(i == appCount) {
                i = 0;
            }
            displayedAppLimit--;
        }
        this->app_context->display->updateDisplay();
        Serial.println("=========================================\r\n");
    }

    void onProgress() override {
        Time* t = app_context->currentTime;
        
        char time_senc[16];
        sprintf(time_senc,"%02d:%02d:%02d",t->hour,t->min,t->sec);
        this->app_context->display->fillRect(75,1,52,10,0);
        this->app_context->display->drawStringRight(1,time_senc,1);

        char date_senc[11];
        sprintf(time_senc,"%02d:%02d:%02d",t->date,t->month,t->year);
        this->app_context->display->fillRect(1,1,52,10,0);
        this->app_context->display->drawString(1,1,time_senc,1);

        this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;    
    
    void onExit() override {
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }


};

#endif