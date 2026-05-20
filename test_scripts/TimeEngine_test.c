#include "TimeEngine.h"

using namespace TimeEngine;

void setup() {
    Time t;
    t.year = 2024;
    t.month = 5;
    t.date = 18;
    t.hour = 23;
    t.min = 45;
    t.sec = 30;
    Serial.begin(9600);
    Serial.print("Current Back: ");
    Serial.print(t.year); Serial.print("-");
    Serial.print(t.month); Serial.print("-");
    Serial.print(t.date); Serial.print(" ");
    Serial.print(t.hour); Serial.print(":");
    Serial.print(t.min); Serial.print(":");
    uint32_t epoch = convertDate2Epoch(&t);
    Serial.print("Epoch Time: ");
    Serial.println(epoch);
    Time convertedBack = convertEpoch2Time(epoch);
    Serial.print("Converted Back: ");
    Serial.print(convertedBack.year); Serial.print("-");
    Serial.print(convertedBack.month); Serial.print("-");
    Serial.print(convertedBack.date); Serial.print(" ");
    Serial.print(convertedBack.hour); Serial.print(":");
    Serial.print(convertedBack.min); Serial.print(":");
    Serial.println(convertedBack.sec);
    uint8_t dayOfWeek = getDayOfWeek(&t);
    Serial.print("Day of Week: ");
    Serial.print(dayOfWeek);
    Serial.print(", ");
    switch(dayOfWeek) {
        case 0: Serial.println("Monday"); break;
        case 1: Serial.println("Tuesday"); break;
        case 2: Serial.println("Wednesday"); break;
        case 3: Serial.println("Thursday"); break;
        case 4: Serial.println("Friday"); break;
        case 5: Serial.println("Saturday"); break;
        case 6: Serial.println("Sunday"); break;
    }

    Event event;
    event.eventTime = t;
    event.flags = 0b00010100; // Reminder: 1 month before, daily repeat
    event.repeatInterval = 1;
    Time reminderTime = applyReminderOffset(&event);
    Serial.print("Reminder Time: ");
    Serial.print(reminderTime.year); Serial.print("-");
    Serial.print(reminderTime.month); Serial.print("-");
    Serial.print(reminderTime.date); Serial.print(" ");
    Serial.print(reminderTime.hour); Serial.print(":");
    Serial.print(reminderTime.min); Serial.print(":");
    Serial.println(reminderTime.sec);
    Time repeatTime = applyRepeatOffset(&event);
    Serial.print("Repeat Time: ");
    Serial.print(repeatTime.year); Serial.print("-");
    Serial.print(repeatTime.month); Serial.print("-");
    Serial.print(repeatTime.date); Serial.print(" ");
    Serial.print(repeatTime.hour); Serial.print(":");
    Serial.print(repeatTime.min); Serial.print(":");
    Serial.println(repeatTime.sec);

    if(compareDateTime(&t, &reminderTime) > 0) {
        Serial.println("Event is in the future compared to reminder time.");
    }
    if(compareDateTime(&t, &repeatTime) < 0) {
        Serial.println("Event is in the past compared to repeat time.");
    }
    if(compareDateTime(&t, &convertedBack) == 0) {
        Serial.println("Original time and converted back time are equal.");
    }
}

void loop() {

}