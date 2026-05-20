#include "DataModels.h"
#include "TimeEngine.h"
#include "StorageManager.h"
#include <string>

StorageManager storage;

void setup() {
    Serial.begin(115200);
    switch (storage.initFS())
    {
    case -1:
        Serial.printf("LittleFS failed to initialise. Attempting disk format.\r\n");
        storage.initFS(true);
        break;
    case 0:
        break;
    case 1:
        Serial.printf("calendarHeader.bin doesn't open.\r\n");
        break;
    case 2:
        Serial.printf("active.bin doesn't open.\r\n");
        break;
    case 3:
        Serial.printf("calendarHeader.bin and active.bin don't open.\r\n");
        break;
    default:
        Serial.printf("Unexpected output. Files initialisation failed.\r\n");
        break;
    }
    for(uint8_t i = 0; i < 10; i++) {
        Event e = {0};
        snprintf(e.name, sizeof(e.name), "Event%d", i + 1);
        snprintf(e.details, sizeof(e.details), "Event%d_details", i + 1);
        e.eventTime.date = 10 + i;
        e.eventTime.month = 5;
        e.eventTime.year = 2026;
        e.flags = (i % 6) << 3;
        e.repeatInterval = i + 1;
        switch(storage.saveEvent(&e)) {
            case -1:
                Serial.printf("Event limit reached.\r\n");
                break;
            case 0:
                Serial.printf("Event saved successfully.\r\n");
                break;
            case 1:
                Serial.printf("Files open but failed to write in active.bin.\r\n");
                break;
            case 2:
                Serial.printf("Files open but failed to write in calendarHeader.bin.\r\n");
                break;
            case 4:
                Serial.printf("Failed to open active.bin.\r\n");
                break;
            case 8:
                Serial.printf("Failed to open calendarHeader.bin.\r\n");
                break;
            case 12:
                Serial.printf("Failed to open both files.\r\n");
                break;
            default:
                Serial.printf("Unrecognised return value. Function fail.\r\n");
                break;
        }
    }
    Serial.printf("\r\n");
    storage.printDebugState();
    Serial.printf("----Delete event with ID 4----\r\n");
    uint16_t deletionID = 4;
    switch(storage.deleteEventByID(deletionID)) {
        case -1:
            Serial.printf("Event with ID = %d not found.\r\n", deletionID);
            break;
        case 0:
            Serial.printf("Event with ID = %d successfully deleted.", deletionID);
            break;
        case 1:
            Serial.printf("Files open but failed to write in active.bin.\r\n");
            break;
        case 2:
            Serial.printf("Files open but failed to write in calendarHeader.bin.\r\n");
            break;
        case 4:
            Serial.printf("Failed to open active.bin.\r\n");
            break;
        case 8:
            Serial.printf("Failed to open calendarHeader.bin.\r\n");
            break;
        case 12:
            Serial.printf("Failed to open both files.\r\n");
            break;
        default:
            Serial.printf("Unrecognised return value. Function fail.\r\n");
            break;
    }
    storage.printDebugState();
}

void loop() {

}