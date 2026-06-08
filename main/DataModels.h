#ifndef DATAMODELS_H
#define DATAMODELS_H

// ==========================================
// TIME STRUCTURE (7 Bytes)
// ==========================================
/**
 * @struct Time
 * @brief Time struct which includes date and time.
 * @details It uses bit packing to eliminate padding to minimize space occupied.
 */
typedef struct {
  /** valid years from 2000 to 2999, occupies 2 bytes */
  uint16_t year;
  /** valid from 1 to 12 corresponding from January to December, occupies 1 byte */
  uint8_t month;
  /** valid from 1 to last date of corresponding month, occupies 1 byte */
  uint8_t date;
  /** valid from 0 to 23 following the 24-hour format, occupies 1 byte */
  uint8_t hour;
  /** valid from 0 to 59, occupies 1 byte */
  uint8_t min;
  /** valid from 0 to 59, occupies 1 byte */
  uint8_t sec;
} __attribute__((packed)) Time;

// ==========================================
// FILE SYSTEM HEADER (6 Bytes)
// ==========================================
/**
 * @struct FileHeader
 * @brief Application header struct which includes number of events and last event ID assigned.
 * @details 
 * - It uses an incremental ID assignment logic to ensure that each event has a new ID assigned.
 * - It also uses bit packing to eliminate padding to minimize space occupied.
 */
typedef struct {
  /** version of the application, occupies 2 bytes */
  uint16_t version;
  /** total number of events in the memory, occupies 2 bytes */
  uint16_t totalEvents;
  /** last event ID assigned, occupies 2 bytes */
  uint16_t currentEventID;
} __attribute__((packed)) FileHeader;

// ==========================================
// EVENT STRUCTURE (60 Bytes)
// ==========================================
/**
 * @struct Event
 * @brief Event struct which includes event datetime, name, detail, reminding and reccuring attributes.
 * @details It uses an object of the Time struct to store datetime and a flag based architecture to store reminder and repeat attributes.
 * - It is used to store a detailed description of an event being stored.
 * - It also uses bit packing to eliminate padding to minimize space occupied.
 */
typedef struct {
  /** event unique identifier, occupes 2 bytes */
  uint16_t id;                  // 2 bytes
  /** event name, occupies 16 bytes/characters */
  char name[16];                // 16 bytes
  /** event details like place, people, phonenumber etc., occupies 32 bytes/characters */
  char details[32];             // 32 bytes
  /** event datetime object which stores datetime for alarm, occupies as is the size of Time struct */
  Time eventTime;               // 7 bytes
  
  // BITMASK: [7-6: State] | [5-3: Repeat Type] | [2-0: Reminder Type]
  /**
 * <table>
 * <tr>
 * <th align="center">Bits</th>
 * <th align="center">Function</th>
 * <th align="center">Options</th>
 * <th align="center">Remarks</th>
 * </tr>
 * <tr>
 * <td align="center" rowspan="7">[5-3]</td align="center">
 * <td align="center" rowspan="7">Repeat Type</td align="center">
 * <td align="center">No repeat</td align="center">
 * <td align="center"> - </td align="center">
 * </tr>
 * <tr><td align="center">1 hour repeat</td align="center"><td align="center">multiplied with @ref repeatInterval, to get n-hour repeat</td align="center"></tr>
 * <tr><td align="center">Daily repeat</td align="center"><td align="center">multiplied with @ref repeatInterval, to get n-days repeat</td align="center"></tr>
 * <tr><td align="center">Weekly repeat</td align="center"><td align="center">multiplied with @ref repeatInterval, to get n-week repeat</td align="center"></tr>
 * <tr><td align="center">Monthly repeat</td align="center"><td align="center">multiplied with @ref repeatInterval, to get n-month repeat</td align="center"></tr>
 * <tr><td align="center">Yearly repeat</td align="center"><td align="center">multiplied with @ref repeatInterval, to get n-year repeat</td align="center"></tr>
 * <tr><td align="center">Custom week repeat</td align="center"><td align="center">multiplied with @ref repeatInterval, to get n-week repeat on days in the @ref customRepeatDays</td align="center"></tr>
 * <tr>
 * <td align="center" rowspan="5">[2-0]</td align="center">
 * <td align="center" rowspan="5">Reminder Type</td align="center">
 * <td align="center">No reminder</td align="center">
 * <td align="center"> - </td align="center">
 * </tr>
 * <tr><td align="center">1 hour before</td align="center"><td align="center"> - </td align="center"></tr>
 * <tr><td align="center">1 day before</td align="center"><td align="center"> - </td align="center"></tr>
 * <tr><td align="center">1 week before</td align="center"><td align="center"> - </td align="center"></tr>
 * <tr><td align="center">1 month(30 days) before</td align="center"><td align="center"> - </td align="center"></tr>
 * </table>
 */
  uint8_t flags;                // 1 byte 
  /** repetition multiplier which is multiplied to the repeat type */
  uint8_t repeatInterval;       // 1 byte
  /** used for custom repeat i.e. on specific days of the week */
  uint8_t customRepeatDays;     // 1 byte (Bitmask for Mon-Sun)
} __attribute__((packed)) Event;

#endif