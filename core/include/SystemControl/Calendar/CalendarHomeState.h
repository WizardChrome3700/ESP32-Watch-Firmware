#pragma once
#ifndef CALENDARHOMESTATE_H
#define CALENDARHOMESTATE_H

#include "AppState.h"

/**
 * @brief Home state of the Calendar application
 * 
 * @details It performs the following operations:-
 * - It displays the current date and time.
 * - It displays the event that is about to arrive and a progress bar that displays time left before the event arrives.
 * - It displays number of missed events.
 * - It displays the battery level.
 */
class CalendarHomeState : public AppState {
private:
    /**
     * @brief index of event in the alarm queue.
     * 
     * @details index of event in the alarm queue.
     */
    uint8_t displayedEventIndex;
    /**
     * @brief pointer to the time of the event being displayed.
     * @details pointer to the time of the event being displayed.
     */
    const Time* eventTime_pointer;
public:
    /**
     * @brief Construct a new Calendar Home State object
     * 
     * @param app_context 
     * @param display_event_index 
     */
    CalendarHomeState(AppContext* app_context, uint8_t display_event_index) {
        this->app_context = app_context;
        this->displayedEventIndex = display_event_index;
        // FIX 3: No local copies of time or counts! We fetch dynamically on load.
    }

    /**
     * @brief Renders the home screen of the calendar application
     * @details It performs the following operations:-
     * - It displays the current date and time.
     * - It displays the event that is about to arrive.
     * - It displays number of missed events.
     * - It displays the battery level.
     */
    void onEnter() override {
        clearConsole();
        this->app_context->display->clearBuffer();
        Serial.println("CalendarHomestate entered.");
        // Fetch fresh data directly from the system context
        Time* t = app_context->currentTime;
        uint8_t alarmCount = app_context->alarm_manager->getAlarmCount();
        const AlarmNode* alarmQueue = app_context->alarm_manager->getAlarmQueue();
        const Event* eventsArray = app_context->storage_manager->getEventsArray();
        uint16_t totalEvents = app_context->storage_manager->getTotalEvents();
        uint8_t missedCount = app_context->alarm_manager->getMissedCount();

        Serial.println("=========================================");
        Serial.println("         [ CALENDAR HOME SCREEN ]        ");
        Serial.println("=========================================");
        Serial.printf(" TIME: %02d:%02d:%02d\r\n", t->hour, t->min, t->sec);
        Serial.printf(" DATE: %02d/%02d/%d\r\n", t->date, t->month, t->year);
        Serial.println("-----------------------------------------");
        
        if (alarmCount > displayedEventIndex) {
            uint16_t currentID = alarmQueue[displayedEventIndex].eventID;
            for(uint16_t i=0; i < totalEvents; i++) {
                if (eventsArray[i].id == currentID) {
                    Serial.printf(" UPCOMING: %s\r\n", eventsArray[i].name);
                    eventTime_pointer = &eventsArray[i].eventTime;
                    this->app_context->display->drawStringCentered(47, eventsArray[i].name, 1);
                    break;
                }
                else {
                    eventTime_pointer = nullptr;
                }
            }
        } else {
            Serial.println(" UPCOMING: None");
            eventTime_pointer = nullptr;
            this->app_context->display->drawStringCentered(47, "None", 1);
        }
        
        Serial.println("-----------------------------------------");
        if (missedCount > 0) {
            Serial.printf(" *** %d MISSED EVENT(S)! ***\r\n", missedCount);
        } else {
            Serial.println(" No Missed Events.");
        }
        Serial.println("=========================================\r\n");
        char missed_senc[13];
        char battery_senc[5];
        // char missed_senc[13] = "MISSED: 0";
        // char battery_senc[5] = "100%";
        sprintf(missed_senc, "MISSED: %d", missedCount);
        sprintf(battery_senc, "%d%%", 100);
        this->app_context->display->drawStringRight(1, missed_senc, 1);
        this->app_context->display->drawString(1, 1, battery_senc, 1);
        this->app_context->display->drawStringCentered(37, "UPCOMING", 1);
        // this->app_context->display->drawStringCentered(48, "meeting with sir", 1); // WIP: to be removed
        
    }

    /**
     * @brief Renders the progress bar on the display
     * @details Implementation:-
     * - It obtains the time epoch for the event and the time epoch of current time obtained from RTC.
     * - It obtains the relative fraction of time till event time epoch is reached which is multiplied with display width.
     * - In case there are no upcoming events it displays a dashed line.
     * @note The hardcoding of the progress bar length needs to be rectified to be generalised.
     */
    void onProgress() override {
        Time* t = app_context->currentTime;
        char time_senc[16];
        // char time_senc[9] = "12:00:00"; 
        sprintf(time_senc, "%02d:%02d:%02d", t->hour, t->min, t->sec);
        this->app_context->display->fillRect(34, 27, 54, 8, 0);
        this->app_context->display->drawStringCentered(27, time_senc, 1);
        this->app_context->display->drawLine(1, 62, 127, 62, 0);
        if(eventTime_pointer == nullptr) {
            this->app_context->display->drawLine(  1,  62,  10, 62, 1);
            this->app_context->display->drawLine( 16,  62,  20, 62, 1);
            this->app_context->display->drawLine( 26,  62,  30, 62, 1);
            this->app_context->display->drawLine( 36,  62,  40, 62, 1);
            this->app_context->display->drawLine( 46,  62,  50, 62, 1);
            this->app_context->display->drawLine( 56,  62,  60, 62, 1);
            this->app_context->display->drawLine( 66,  62,  70, 62, 1);
            this->app_context->display->drawLine( 76,  62,  80, 62, 1);
            this->app_context->display->drawLine( 86,  62,  90, 62, 1);
            this->app_context->display->drawLine( 96,  62, 100, 62, 1);
            this->app_context->display->drawLine(106,  62, 110, 62, 1);
            this->app_context->display->drawLine(116,  62, 120, 62, 1);
        }
        else {
            float progressBarLength = (float)(convertDate2Epoch(eventTime_pointer) - convertDate2Epoch(t))*127.0/(convertDate2Epoch(eventTime_pointer) - *(app_context->lastAlarmEpoch));
            this->app_context->display->drawLine(1, 62, (uint8_t)progressBarLength, 62, 1);
            // Serial.println((uint8_t)progressBarLength);
        }
        this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;

    /**
     * @brief it is invoked to exit from the Calendar application
     * @details Implementation:-
     * - It clears the display.
     */
    void onExit() override {
        Serial.println("Clear screen");
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

#endif