#ifndef TIMEENGINE_H
#define TIMEENGINE_H

#include <Arduino.h>
#include "DataModels.h"

namespace TimeEngine
{

uint32_t convertDate2Epoch(Time* t) {
    // Days elapsed at the start of each month (non-leap year)
    static const uint16_t monthDays[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    uint16_t year = t->year;
    uint8_t month = t->month;
    uint8_t date = t->date;
    uint8_t hour = t->hour;
    uint8_t min = t->min;
    uint8_t sec = t->sec;

    // Calculate total days from 1970 to the given year
    uint32_t days = (year - 1970) * 365 + ((year - 1969) / 4); // Add leap years
    // Add days for the months in the current year
    days += monthDays[month - 1];
    // Add one more day if it's a leap year and we're past February
    if (month > 2 && (year % 4 == 0)) {
        days += 1;
    }
    // Add days for the current month
    days += (date - 1);
    
    // Convert total days to seconds and add time components
    return (days * 86400UL) + (hour * 3600UL) + (min * 60UL) + sec;
}

Time convertEpoch2Time(uint32_t epoch) {
    Time t;
    // Days elapsed at the start of each month (non-leap year)
    static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    // 1. Extract raw time components from the remainder of the day
    uint32_t secondsInDay = epoch % 86400UL;
    t.sec  = secondsInDay % 60U;
    t.min  = (secondsInDay % 3600UL) / 60U;
    t.hour = secondsInDay / 3600UL;
    // 2. Convert epoch seconds into total days elapsed since 1970
    uint32_t days = epoch / 86400UL;
    // 3. Find the Year
    uint16_t year = 1970;
    while (true) {
        // Check if the current year candidate is a leap year
        bool isLeap = (year % 4 == 0); 
        uint16_t daysInYear = isLeap ? 366 : 365;

        if (days >= daysInYear) {
            days -= daysInYear;
            year++;
        } else {
            break; // Found the correct year
        }
    }
    t.year = year;
    // 4. Find the Month and Date
    uint8_t month = 0;
    while (true) {
        // Handle February expansion during leap years
        uint8_t daysInMonth = monthDays[month];
        if (month == 1 && (year % 4 == 0)) {
            daysInMonth = 29;
        }
        if (days >= daysInMonth) {
            days -= daysInMonth;
            month++;
        } else {
            break; // Found the correct month
        }
    }
    t.month = month + 1;       // Convert 0-indexed month to 1-12
    t.date  = (uint8_t)days + 1; // Remaining days represent the day of the month (1-31)
    return t;
}

inline uint8_t getDayOfWeek(Time* t) {
    const uint8_t t_adj[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t y = t->year - (t->month < 3);
    return (y + y/4 - y/100 + y/400 + t_adj[t->month-1] + t->date + 1) % 7;
}

inline int8_t compareDateTime(Time* a, Time* b) {
    // return +1 if a in future, -1 if a in past and 0 if both are equal
    // 1. Compare the 16-bit years first
    if (a->year != b->year) {
        return (a->year > b->year) ? 1 : -1;
    }
    
    uint32_t packedA = ((uint32_t)a->month << 24) | 
                  ((uint32_t)a->date  << 16) | 
                  ((uint32_t)a->hour  << 8)  | 
                   (uint32_t)a->min;
    uint32_t packedB = ((uint32_t)b->month << 24) | 
                  ((uint32_t)b->date  << 16) | 
                  ((uint32_t)b->hour  << 8)  | 
                   (uint32_t)b->min;
    if (packedA > packedB) {
        return 1; // a is in the future
    } else if (packedA < packedB) {
        return -1; // a is in the past
    } else {
        if(a->sec > b->sec) {
            return 1; // a is in the future
        } else if(a->sec < b->sec) {
            return -1; // a is in the past
        } else {
            return 0; // both are equal
        }
    }
}

bool isLeapYear(uint16_t year) {
    return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
    uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return monthDays[month - 1];
}

Time applyReminderOffset(Event* event) {
    Time t = event->eventTime;
    uint32_t eventEpoch = convertDate2Epoch(&t);
    uint8_t reminderType = event->flags & 0b00000111; // Extract the last 3 bits for reminder type
    Serial.print("Reminder Type: ");Serial.println(reminderType);
    switch(reminderType) {
        case 0:
            return t; // No reminder
        case 1:
            eventEpoch -= 60 * 60; // 1 hour before
            break;
        case 2:
            eventEpoch -= 24 * 60 * 60; // 1 day before
            break;
        case 3:
            eventEpoch -= 7 * 24 * 60 * 60; // 1 week before
            break;
        case 4:
            eventEpoch -= 30 * 24 * 60 * 60; // 1 month before (approximate as 30 days)
            break;
    }
    return convertEpoch2Time(eventEpoch);
}

Time applyRepeatOffset(Event* event) {
    Time t = event->eventTime;
    uint32_t eventEpoch = convertDate2Epoch(&t);
    uint8_t repeatType = (event->flags & 0b00111000) >> 3; // Extract bits 3-5 for repeat type
    Serial.print("Repeat Type: ");Serial.println(repeatType);
    switch(repeatType) {
        case 0:
            return t; // No repeat
        case 1:
            eventEpoch += 60 * 60 * event->repeatInterval; // Repeat n every hours
            break;
        case 2:
            eventEpoch += 24 * 60 * 60 * event->repeatInterval; // Repeat every n days
            break;
        case 3:
            eventEpoch += 7 * 24 * 60 * 60 * event->repeatInterval; // Repeat every n weeks
            break;
        case 4:
            // 1. Calculate how many years we need to add based on month overflow
            t.year += (t.month + event->repeatInterval - 1) / 12;
            
            // 2. Wrap the month safely to 1-12
            t.month = ((t.month + event->repeatInterval - 1) % 12) + 1;
            
            // 3. Clamp the date (e.g. prevent Feb 31st)
            if(t.date > daysInMonth(t.year, t.month)) {
                t.date = daysInMonth(t.year, t.month);
            }
            return t; // Safe to return directly!
            
        case 5:
            t.year += event->repeatInterval; // Repeat every n years
             if(t.month == 2 && t.date == 29 && !isLeapYear(t.year)) {
                t.date = 28; // Adjust for non-leap year
            }
            return t; // Safe to return directly!
            
        case 6:
        {
            uint8_t customDaysBitMask = event->customRepeatDays;
            uint8_t currentDayOfWeek = getDayOfWeek(&t);
            for(uint8_t i = 1; i <= 7; i++) {
                if((customDaysBitMask >> ((currentDayOfWeek + i) % 7)) & 1) {
                    
                    uint32_t daysToAdd = i;
                    
                    // If we wrapped into a new week AND interval > 1, add the extra weeks!
                    if (((currentDayOfWeek + i) % 7) <= currentDayOfWeek && event->repeatInterval > 1) {
                        daysToAdd += 7 * (event->repeatInterval - 1);
                    }
                    
                    // Add the exact seconds to the epoch
                    eventEpoch += daysToAdd * 24 * 60 * 60;
                    break;
                }
            }
            // Do NOT return t here. Let it fall through to convertEpoch2Time!
            break;
        }
    }
    return convertEpoch2Time(eventEpoch);
}

}

#endif