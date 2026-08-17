#pragma once
#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "driver/gpio.h"

// Pin States
#define LOW  0
#define HIGH 1

// Arduino-style Pin Modes
typedef enum {
    INPUT,
    OUTPUT,
    INPUT_PULLUP,
    INPUT_PULLDOWN
} pin_mode_t;

// Enable C-compatible compilation bindings if processed by a C++ compiler
#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
esp_err_t pinMode(uint8_t pin, pin_mode_t mode);
esp_err_t digitalWrite(uint8_t pin, uint8_t val);
uint8_t digitalRead(uint8_t pin);
esp_err_t analogWrite(uint8_t pin, uint32_t duty);
esp_err_t isValidPin(uint8_t pin);
void delayMicroseconds(uint32_t us);
void delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif // GPIO_H
