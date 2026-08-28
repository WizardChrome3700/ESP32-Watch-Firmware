#pragma once
#ifndef WIFISTATE_H
#define WIFISTATE_H
#include "AppState.h"

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
    void onExit() override { 
        Serial.println("[SYSTEM] Turning off WiFi Module...");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

#endif