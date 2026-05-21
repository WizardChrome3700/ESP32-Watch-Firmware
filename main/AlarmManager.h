#ifndef ALARMMANAGER_H
#define ALARMMANAGER_H

#include "DataModels.h"
#include "StorageManager.h"
#include "TimeEngine.h"
#include "RTC_DS3231.h"

#define MAX_ALARM_QUEUE 100 // Cap to save RAM
#define MAX_MISSED_QUEUE 10 // Catch limit for simultaneous missed events

using namespace TimeEngine;

struct AlarmNode {
    uint16_t eventID;
    uint32_t triggerEpoch; 
    bool isReminder;       // true = pre-emption warning, false = main event
};

class AlarmManager {
    private:
        // The Future Queue
        AlarmNode alarmQueue_[MAX_ALARM_QUEUE];
        uint16_t queueSize_;
        // The Leeway/Missed Queue
        AlarmNode missedQueue_[MAX_MISSED_QUEUE];
        uint8_t missedCount_;
        // Internal helper for sorting the arrays by Epoch time
        void sortQueueChronologically(AlarmNode* queue, uint16_t queue_size);

    public:
        // Core Engine
        void rebuildQueue(const Event* eventsArray, uint16_t totalEvents, uint32_t currentEpoch);
        // RTC Interfacing
        void programNextAlarm(RTC_DS3231* rtc);
        // UI Handoffs
        uint8_t getMissedCount() const { return missedCount_; }
        const AlarmNode* getMissedQueue() const { return missedQueue_; }
        // Sanity Checks
        void printQueueDebug();
};

void AlarmManager::sortQueueChronologically(AlarmNode* queue, uint16_t queue_size) {
    for(uint16_t sort_index = 0; sort_index < queue_size; sort_index++) {
        uint32_t min_time_eventEpoch = queue[sort_index].triggerEpoch;
        uint16_t min_time_eventIndex = sort_index;
        for(uint16_t search_index = sort_index + 1; search_index < queue_size; search_index++) {
            if(queue[search_index].triggerEpoch < min_time_eventEpoch) {
                min_time_eventEpoch = queue[search_index].triggerEpoch;
                min_time_eventIndex = search_index;
            }
        }
        AlarmNode temp_node = queue[min_time_eventIndex];
        queue[min_time_eventIndex] = queue[sort_index];
        queue[sort_index] = temp_node;
    }
}

void AlarmManager::rebuildQueue(const Event* eventsArray, uint16_t totalEvents, uint32_t currentEpoch) {
    this->queueSize_ = 0;
    this->missedCount_ = 0;
    Time currentEndTime = convertEpoch2Time(currentEpoch);
    currentEndTime.hour = 23;
    currentEndTime.min = 59;
    currentEndTime.sec = 59;
    uint32_t currentEndEpoch = convertDate2Epoch(&currentEndTime);

    for(uint16_t i = 0; i < totalEvents; i++) {
        Event event = eventsArray[i];
        uint32_t eventEpoch = convertDate2Epoch(&event.eventTime);
        Time eventReminderTime = applyReminderOffset(&event);
        uint32_t eventReminderEpoch = convertDate2Epoch(&eventReminderTime);
        Serial.printf("Event ID: %d | EventEpoch: %u | CurrEpoch: %u | EndEpoch: %u\n", 
              eventsArray[i].id, eventEpoch, currentEpoch, currentEndEpoch);
        if((eventEpoch > currentEpoch) && (eventEpoch < currentEndEpoch)) {
            this->alarmQueue_[this->queueSize_].eventID = eventsArray[i].id;
            this->alarmQueue_[this->queueSize_].triggerEpoch = eventEpoch;
            this->alarmQueue_[this->queueSize_].isReminder = false;
            this->queueSize_++;
        }
        else if((currentEpoch >= eventEpoch) && (currentEpoch <= eventEpoch + 120) && (eventEpoch < currentEndEpoch)) {
            this->missedQueue_[this->missedCount_].eventID = eventsArray[i].id;
            this->missedQueue_[this->missedCount_].triggerEpoch = eventEpoch;
            this->missedQueue_[this->missedCount_].isReminder = false;
            this->missedCount_++;
        }
        if(eventReminderEpoch != eventEpoch) {
            if((eventReminderEpoch > currentEpoch) && (eventReminderEpoch < currentEndEpoch)) {
                this->alarmQueue_[this->queueSize_].eventID = eventsArray[i].id;
                this->alarmQueue_[this->queueSize_].triggerEpoch = eventReminderEpoch;
                this->alarmQueue_[this->queueSize_].isReminder = true;
                this->queueSize_++;
            }
            else if((currentEpoch >= eventReminderEpoch) && (currentEpoch <= eventReminderEpoch + 120) && (eventReminderEpoch < currentEndEpoch)) {
                this->missedQueue_[this->missedCount_].eventID = eventsArray[i].id;
                this->missedQueue_[this->missedCount_].triggerEpoch = eventReminderEpoch;
                this->missedQueue_[this->missedCount_].isReminder = true;
                this->missedCount_++;
            }
        }
    }
    this->sortQueueChronologically(this->alarmQueue_, this->queueSize_);
    this->sortQueueChronologically(this->missedQueue_, this->missedCount_);
}

void AlarmManager::programNextAlarm(RTC_DS3231* rtc) {
    rtc->clearAlarm1();
    if(this->queueSize_ != 0) {
        uint32_t nextAlarmEpoch = this->alarmQueue_[0].triggerEpoch;
        Time nextAlarmTime = convertEpoch2Time(nextAlarmEpoch);
        Serial.printf("[DEBUG] Software trying to program RTC for -> %02d:%02d:%02d on Date: %02d\n", 
                  nextAlarmTime.hour, nextAlarmTime.min, nextAlarmTime.sec, nextAlarmTime.date);
        rtc->setAlarm(nextAlarmTime.hour, nextAlarmTime.min, nextAlarmTime.sec, nextAlarmTime.date);
        rtc->enableAlarm1();
    }
    else {
        Serial.println("[DEBUG] queueSize_ is 0! Disabling RTC Interrupt.");
        rtc->disableAlarm1();
    }
}

#endif