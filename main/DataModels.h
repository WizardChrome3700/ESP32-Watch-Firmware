#ifndef DATAMODELS_Hgut
#define DATAMODELS_H

// ==========================================
// TIME STRUCTURE (7 Bytes)
// ==========================================
typedef struct {
  uint16_t year;    // 2 bytes
  uint8_t month;    // 1 byte
  uint8_t date;     // 1 byte
  uint8_t hour;     // 1 byte
  uint8_t min;      // 1 byte
  uint8_t sec;      // 1 byte
} __attribute__((packed)) Time;

// ==========================================
// FILE SYSTEM HEADER (6 Bytes)
// ==========================================
typedef struct {
  uint16_t version;         // 2 bytes
  uint16_t totalEvents;     // 2 bytes
  uint16_t currentEventID;  // 2 bytes
} __attribute__((packed)) FileHeader;

// ==========================================
// EVENT STRUCTURE (60 Bytes)
// ==========================================
typedef struct {
  uint16_t id;                  // 2 bytes
  char name[16];                // 16 bytes
  char details[32];             // 32 bytes
  Time eventTime;               // 7 bytes
  
  // BITMASK: [7-6: State] | [5-3: Repeat Type] | [2-0: Reminder Type]
  uint8_t flags;                // 1 byte 
  
  uint8_t repeatInterval;       // 1 byte
  uint8_t customRepeatDays;     // 1 byte (Bitmask for Mon-Sun)
} __attribute__((packed)) Event;

#endif