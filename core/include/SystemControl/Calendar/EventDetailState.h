#pragma once
#ifndef EVENTDETAILSTATE_H
#define EVENTDETAILSTATE_H
#include "AppState.h"

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
        Serial.println("=========================================");
        Serial.println("           [ EVENT DETAILS ]             ");
        Serial.println("=========================================");
        this->app_context->display->drawStringCentered(1, "EVENT DETAILS", 1);
        if(event_pointer != nullptr) {
            Serial.printf(" Name:    %s\r\n", event_pointer->name);
            this->app_context->display->drawStringCentered(20, event_pointer->name, 1);
            Serial.printf(" Time:    %02d:%02d:%02d\r\n", event_pointer->eventTime.hour, event_pointer->eventTime.min, event_pointer->eventTime.sec);
            char time_string[12];
            sprintf(time_string, "%02d:%02d:%02d", event_pointer->eventTime.hour, event_pointer->eventTime.min, event_pointer->eventTime.sec);
            this->app_context->display->drawString(1, 30, time_string, 1);
            Serial.printf(" Date:    %02d/%02d/%d\r\n", event_pointer->eventTime.date, event_pointer->eventTime.month, event_pointer->eventTime.year);
            sprintf(time_string, "%02d/%02d/%02d", event_pointer->eventTime.date, event_pointer->eventTime.month, event_pointer->eventTime.year);
            this->app_context->display->drawStringRight(30, time_string, 1);
            Serial.println("-----------------------------------------");
            Serial.printf(" Details: %s\r\n", event_pointer->details);
            this->app_context->display->drawStringWrapped(1, 40, event_pointer->details, 1);
        } else {
            Serial.println(" Error: Event Not Found.");
        }
        Serial.println("=========================================\r\n");
        this->app_context->display->updateDisplay();
    }

    void onProgress() override { return; }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override { 
        Serial.println("Clear screen");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

#endif