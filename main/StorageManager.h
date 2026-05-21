#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <LittleFS.h>
#include "TimeEngine.h"
#include "DataModels.h"

#define MAX_EVENTS 500

class StorageManager {
    private:
    FileHeader calendarHeader_;
    Event eventsArray_[MAX_EVENTS];

    public:
    int8_t initFS(bool format);
    int8_t saveEvent(Event* event);
    int8_t deleteEventByID(uint16_t targetID);
    void printDebugState();
    const Event* getEventsArray() const { return eventsArray_; }
    uint16_t getTotalEvents() const { return calendarHeader_.totalEvents; }
};

int8_t StorageManager::initFS(bool format = false) {
    if(!LittleFS.begin(format)) {
        return -1;
    }
    else {
        int8_t files_exist = ((uint8_t)LittleFS.exists("/calendarHeader.bin") << 1) | ((uint8_t)LittleFS.exists("/active.bin"));
        int8_t files_dont_open = 0;
        if(files_exist == 0) {
            File calendar_header_file = LittleFS.open("/calendarHeader.bin", "w");
            if(!calendar_header_file) {
                files_dont_open += 1;
            }
            File active_events_file = LittleFS.open("/active.bin", "w");
            if(!active_events_file) {
                files_dont_open += 2;
            }
            if(!files_dont_open) {
                calendarHeader_.version = 1;
                calendarHeader_.totalEvents = 0;
                calendar_header_file.write((uint8_t*)&calendarHeader_, sizeof(calendarHeader_));
            }
            calendar_header_file.close();
            active_events_file.close();
        }
        else if(files_exist == 1) {
            File calendar_header_file = LittleFS.open("/calendarHeader.bin", "w");
            if(!calendar_header_file) {
                files_dont_open += 1;
            }
            if(!files_dont_open) {
                calendarHeader_.version = 1;
                calendarHeader_.totalEvents = 0;
                calendar_header_file.write((uint8_t*)&calendarHeader_, sizeof(calendarHeader_));
            }
            calendar_header_file.close();
        }
        else if(files_exist == 2) {
            File active_events_file = LittleFS.open("/active.bin", "w");
            if(!active_events_file) {
                files_dont_open += 2;
            }
            active_events_file.close();
        }
        else if(files_exist == 3) {
            File calendar_header_file = LittleFS.open("/calendarHeader.bin", "r");
            if(!calendar_header_file) {
                files_dont_open += 1;
            }
            File active_events_file = LittleFS.open("/active.bin", "r");
            if(!active_events_file) {
                files_dont_open += 2;
            }
            if(!files_dont_open) {
                calendar_header_file.read((uint8_t*)&calendarHeader_, sizeof(calendarHeader_));
                for(int i = 0; i < calendarHeader_.totalEvents; i++) {
                    active_events_file.read((uint8_t*)&eventsArray_[i], sizeof(Event));
                }
            }
            calendar_header_file.close();
            active_events_file.close();
        }
        return files_dont_open;
    }
}

int8_t StorageManager::saveEvent(Event* newEvent) {
    if(!(calendarHeader_.totalEvents < MAX_EVENTS)) {
        return -1;
    }
    else {        
        File active_events_file = LittleFS.open("/active.bin", "ab");
        File calendar_header_file = LittleFS.open("/calendarHeader.bin", "w");
        int8_t files_dont_open = 0;
        int8_t files_dont_write = 0;
        if(!active_events_file) {
            files_dont_open += 1;
        }
        if(!calendar_header_file) {
            files_dont_open += 2;
        }
        if(!files_dont_open) {
            newEvent->id = calendarHeader_.currentEventID + 1;
            size_t bytes_new_event = active_events_file.write((uint8_t*)(newEvent), sizeof(*newEvent));
            if(bytes_new_event != sizeof(*newEvent)) {
                files_dont_write += 1;
            }
            else {
                calendarHeader_.totalEvents += 1;
                calendarHeader_.currentEventID += 1;
                eventsArray_[calendarHeader_.totalEvents - 1] = *newEvent;
                size_t byte_cal_header = calendar_header_file.write((uint8_t*)&calendarHeader_, sizeof(calendarHeader_));
                if(byte_cal_header != sizeof(calendarHeader_)) {
                    files_dont_write += 2;
                    active_events_file.seek(active_events_file.size() - sizeof(Event));
                    Event empty_event = {0};
                    active_events_file.write((uint8_t*)(&empty_event), sizeof(empty_event));
                    eventsArray_[calendarHeader_.totalEvents - 1] = empty_event;
                    calendarHeader_.totalEvents -= 1;
                    calendarHeader_.currentEventID -= 1;
                }
            }
        }
        active_events_file.close();
        calendar_header_file.close();
        return (files_dont_open << 2) | files_dont_write;
    }
}

int8_t StorageManager::deleteEventByID(uint16_t targetID) {
    uint16_t eventIndex = 0;
    while((eventIndex < calendarHeader_.totalEvents) && (eventsArray_[eventIndex].id != targetID)) {
        eventIndex++;
    }
    if(eventIndex == calendarHeader_.totalEvents) {
        return -1;
    }
    else {
        Event empty_event = {0};
        Event deleted_event;
        deleted_event = eventsArray_[eventIndex];
        for(uint16_t j = eventIndex; j < calendarHeader_.totalEvents - 1; j++) {
            eventsArray_[j] = eventsArray_[j + 1];
        }
        eventsArray_[calendarHeader_.totalEvents - 1] = empty_event;
        calendarHeader_.totalEvents -= 1;
        File active_events_file = LittleFS.open("/active.bin","w");
        File calendar_header_file = LittleFS.open("/calendarHeader.bin","w");
        int8_t files_dont_open = 0;
        int8_t files_dont_write = 0;
        if(!active_events_file) {
            files_dont_open += 1;
        }
        if(!calendar_header_file) {
            files_dont_open += 2;
        }
        if(!files_dont_open) {
            size_t eventBytes = active_events_file.write((uint8_t*)eventsArray_, sizeof(Event)*calendarHeader_.totalEvents);
            if(eventBytes != sizeof(Event)*calendarHeader_.totalEvents) {
                for(int16_t j = calendarHeader_.totalEvents - 1; j >= eventIndex; j--) {
                    eventsArray_[j + 1] = eventsArray_[j]; 
                }
                eventsArray_[eventIndex] = deleted_event;
                calendarHeader_.totalEvents += 1;
                files_dont_write += 1;
            }
            else {
                size_t bytes_cal_header = calendar_header_file.write((uint8_t*)&calendarHeader_, sizeof(calendarHeader_));
                if(bytes_cal_header != sizeof(calendarHeader_)) {
                    for(int16_t j = calendarHeader_.totalEvents - 1; j >= eventIndex; j--) {
                        eventsArray_[j + 1] = eventsArray_[j]; 
                    }
                    eventsArray_[eventIndex] = deleted_event;
                    calendarHeader_.totalEvents += 1;
                    active_events_file.seek(0);
                    size_t eventBytes = active_events_file.write((uint8_t*)eventsArray_, sizeof(Event)*calendarHeader_.totalEvents);
                    files_dont_write += 2;
                }
            }
        }
        active_events_file.close();
        calendar_header_file.close();
        return (files_dont_open << 2) | files_dont_write;
    }
}

void StorageManager::printDebugState() {
    Serial.printf("Calendar Header:\r\ncurrentEventID = %d\r\ntotalEvents = %d\r\n", calendarHeader_.currentEventID, calendarHeader_.totalEvents);
    for(uint16_t i = 0; i < calendarHeader_.totalEvents; i++) {
        Serial.printf("Event name: %s\r\n", eventsArray_[i].name);
        Serial.printf("Event ID: %d\r\n", eventsArray_[i].id);
        Serial.printf("Event details: %s\r\n", eventsArray_[i].details);
        Serial.printf("Event time: %d-%d-%d, %d:%d:%d\r\n", eventsArray_[i].eventTime.date, 
                        eventsArray_[i].eventTime.month, eventsArray_[i].eventTime.year, 
                        eventsArray_[i].eventTime.hour, eventsArray_[i].eventTime.min, 
                        eventsArray_[i].eventTime.sec);
        uint8_t reminderType = eventsArray_[i].flags & 0b00000111;
        uint8_t repeatType = (eventsArray_[i].flags & 0b00111000) >> 3;
        uint8_t eventState = (eventsArray_[i].flags & 0b11000000) >> 6;
        switch (reminderType)
        {
        case 0:
            Serial.printf("No reminder.\r\n");
            break;
        case 1:
            Serial.printf("1 hour before.\r\n");
            break;
        case 2:
            Serial.printf("1 day before.\r\n");
            break;
        case 3:
            Serial.printf("1 week before.\r\n");
            break;
        case 4:
            Serial.printf("1 month before.\r\n");
            break;
        default:
            Serial.printf("reminder not recognised. corrupted event.\r\n");
            break;
        }
        switch (repeatType)
        {
        case 0:
            Serial.printf("No repetition.\r\n");
            break;
        case 1:
            Serial.printf("%d hour repetition.\r\n", eventsArray_[i].repeatInterval);
            break;
        case 2:
            Serial.printf("%d day repetition.\r\n", eventsArray_[i].repeatInterval);
            break;
        case 3:
            Serial.printf("%d week repetition.\r\n", eventsArray_[i].repeatInterval);
            break;
        case 4:
            Serial.printf("%d month repetition.\r\n", eventsArray_[i].repeatInterval);
            break;
        case 5:
            Serial.printf("%d year repetition.\r\n", eventsArray_[i].repeatInterval);
            break;
        case 6:
            Serial.printf("%d week repetition with following days mask.\r\n", eventsArray_[i].repeatInterval);
            const char* days_in_week[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            for(uint8_t j = 0; j < 7; j++) {
                if((eventsArray_[i].customRepeatDays >> j) & 0b00000001) {
                    Serial.printf("%s ",days_in_week[j]);
                }
            }
            Serial.print("\r\n\r\n");
        }
        // Event State information after we decide behaviour
    }
}

#endif