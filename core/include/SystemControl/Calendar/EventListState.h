#pragma once
#ifndef EVENTLIST_H
#define EVENTLIST_H

#include "AppState.h"

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
        Serial.println("=========================================");
        Serial.println("           [ MISSED EVENTS ]             ");
        Serial.println("=========================================");
        this->app_context->display->clearBuffer();
        this->app_context->display->drawStringCentered(1, "MISSED ALARMS", 1);
        
        // Fetch dynamically!
        const Event* eventsArray = app_context->storage_manager->getEventsArray();
        uint16_t totalEvents = app_context->storage_manager->getTotalEvents();
        const AlarmNode* missedQueue = app_context->alarm_manager->getMissedQueue();
        uint8_t missedCount = app_context->alarm_manager->getMissedCount();
        
        if (missedCount == 0) {
            // Serial.println("  (List is empty)");
            this->app_context->display->drawStringCentered(23, "No missed events.", 1);
        } else {
            uint8_t loopCount = missedCount;
            uint8_t displayLimit = 5;
            for(uint8_t i = displayedEventIndex + 1; (loopCount > 0) && (displayLimit > 0); loopCount--) {
                uint16_t j;
                for(j = 0; j < totalEvents; j++) {
                    if(missedQueue[i-1].eventID == eventsArray[j].id) break;
                }
                if(j < totalEvents) {
                    if (i-1 == displayedEventIndex) {
                        // Serial.printf(" -> %s (%02d:%02d)\r\n", eventsArray[j].name, eventsArray[j].eventTime.hour, eventsArray[j].eventTime.min);
                        this->app_context->display->drawString(1, 15 + (i - 1 - displayedEventIndex)*10, ">", 1);
                        this->app_context->display->drawString(7, 15 + (i - 1 - displayedEventIndex)*10, eventsArray[j].name, 1);
                    } else {
                        // Serial.printf("    %s (%02d:%02d)\r\n", eventsArray[j].name, eventsArray[j].eventTime.hour, eventsArray[j].eventTime.min);
                        // Explicitly cast to (int) to safely perform negative checks on unsigned variables
                        if((int)i - 1 - (int)displayedEventIndex < 0) {
                            // Added the '10 +' base offset
                            this->app_context->display->drawString(7, 15 + (missedCount + i - 1 - displayedEventIndex)*10, eventsArray[j].name, 1);
                        } else {
                            // Added the '10 +' base offset
                            this->app_context->display->drawString(7, 15 + (i - 1 - displayedEventIndex)*10, eventsArray[j].name, 1);
                        }
                    }
                }
                i = i + 1;
                if(i == missedCount + 1) {
                    i = 1;
                }
                displayLimit -= 1;
            }
        }
        this->app_context->display->updateDisplay();
        Serial.println("=========================================\r\n");
    }
    void onProgress() override { 
        return;
    }
    AppState* handleInput(uint8_t buttonPressed) override;
    void onExit() override { 
        Serial.println("Clear screen");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

#endif