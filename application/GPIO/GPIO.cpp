#include "GPIO.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h" // Needed for your precise microsecond delay wrapper

// 1. Enclose ALL target-dependent maps and logic inside the target guard
#if defined(CONFIG_IDF_TARGET_ESP32S3)

esp_err_t isValidPin(uint8_t pin) {
    if (pin == 0  || pin == 3  || pin == 19 || pin == 20 || 
       (pin >= 26 && pin <= 37) || pin == 45 || pin == 46 || pin > 48) {
        // ESP_LOGI(LOG_TAG, "[STATUS]: Invalid pin, Strapping pins not allowed.");
        return ESP_ERR_INVALID_ARG; // Drop immediately if user passes a forbidden pin
    }
    return ESP_OK;
}

#elif defined(CONFIG_IDF_TARGET_ESP32C6)

esp_err_t isValidPin(uint8_t pin) {
    if (pin == 4  || pin == 5  || pin == 8 || pin == 15 || 
       (pin >= 24 && pin <= 20) || pin == 12 || pin == 13) {
        // ESP_LOGI(LOG_TAG, "[STATUS]: Invalid pin, Strapping pins not allowed.");
        return ESP_ERR_INVALID_ARG; // Drop immediately if user passes a forbidden pin
    }
    return ESP_OK;
}

#else
    #error "This custom GPIO framework layout currently only supports the ESP32-S3 target!"
#endif

// The ESP32-S3 provides 8 high-speed hardware channels (LEDC_CHANNEL_0 to LEDC_CHANNEL_7)
#define MAX_PWM_CHANNELS 8

// Structure to track channels that are already set up
typedef struct {
    uint8_t pin;          // Associated raw GPIO pin number
    ledc_channel_t ch;    // Allocated LEDC hardware channel
    bool is_initialized;  // Track configuration state
} pwm_track_t;

// Cache memory to remember channel state across function calls
// Explicitly initialize every single field to zero / defaults across all 8 entries
static pwm_track_t pwm_cache[MAX_PWM_CHANNELS] = {
    {0, LEDC_CHANNEL_0, false}, {0, LEDC_CHANNEL_0, false},
    {0, LEDC_CHANNEL_0, false}, {0, LEDC_CHANNEL_0, false},
    {0, LEDC_CHANNEL_0, false}, {0, LEDC_CHANNEL_0, false},
    {0, LEDC_CHANNEL_0, false}, {0, LEDC_CHANNEL_0, false}
};

static uint8_t next_free_channel_idx = 0;
static bool is_timer_configured = false;

esp_err_t pinMode(uint8_t pin, pin_mode_t mode) {
    // 1. Explicitly protect the forbidden/system hardware zones you highlighted
    esp_err_t status = isValidPin(pin);
    if(status == ESP_ERR_INVALID_ARG) {
        return status;
    }
    
    gpio_num_t real_gpio = (gpio_num_t)pin; // Clean direct integer cast
    
    gpio_reset_pin(real_gpio);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << real_gpio);
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    
    switch (mode) {
        case OUTPUT:          io_conf.mode = GPIO_MODE_OUTPUT; break;
        case INPUT:           io_conf.mode = GPIO_MODE_INPUT; break;
        case INPUT_PULLUP:    io_conf.mode = GPIO_MODE_INPUT; io_conf.pull_up_en = GPIO_PULLUP_ENABLE; break;
        case INPUT_PULLDOWN:  io_conf.mode = GPIO_MODE_INPUT; io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; break;
        default:              return ESP_ERR_INVALID_ARG;
    }

    status |= gpio_config(&io_conf);

    return status;
}

esp_err_t digitalWrite(uint8_t pin, uint8_t val) {
    if (pin > 48) {
        return ESP_ERR_INVALID_ARG;
    }
    
    gpio_num_t real_gpio = (gpio_num_t)pin;
    esp_err_t status = gpio_set_level(real_gpio, (val == LOW) ? 0 : 1);
    return status;
}

uint8_t digitalRead(uint8_t pin) {
    if (pin > 48) {
        return LOW; // Safety return value (Do not leave empty for functions returning data)
    }
    
    gpio_num_t real_gpio = (gpio_num_t)pin;
    return (gpio_get_level(real_gpio) == 0) ? LOW : HIGH;
}

esp_err_t analogWrite(uint8_t pin, uint32_t duty) {
    // 1. Enforce physical hardware constraints for the ESP32-S3
    esp_err_t status = isValidPin(pin);
    if(status == ESP_ERR_INVALID_ARG) {
        return status;
    }

    ledc_channel_t target_channel = LEDC_CHANNEL_MAX;

    // 2. Check if this specific pin was already initialized previously
    for (int i = 0; i < next_free_channel_idx; i++) {
        if (pwm_cache[i].pin == pin && pwm_cache[i].is_initialized) {
            target_channel = pwm_cache[i].ch;
            break;
        }
    }

    // 3. Dynamic initialization block (Runs ONLY ONCE per unique pin)
    if (target_channel == LEDC_CHANNEL_MAX) {
        // Out of hardware channels fallback
        if (next_free_channel_idx >= MAX_PWM_CHANNELS) {
            // ESP_LOGI(LOG_TAG, "[STATUS]: PWM Channels Exhausted");
            status |= ESP_ERR_NOT_FOUND;
            return status; 
        }

        // Configure the shared PWM Timer if it hasn't been done yet
        if (!is_timer_configured) {
            ledc_timer_config_t ledc_timer = {};
            ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE; // S3 uses low-speed mode peripheral matrix
            ledc_timer.duty_resolution  = LEDC_TIMER_8_BIT;    // 0-255 duty cycle spectrum (Standard Arduino layout)
            ledc_timer.timer_num        = LEDC_TIMER_0;
            ledc_timer.freq_hz          = 5000;                // 5kHz PWM Frequency
            ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
            ledc_timer_config(&ledc_timer);
            is_timer_configured = true;
        }

        // Allocate a unique hardware channel to this pin
        target_channel = (ledc_channel_t)next_free_channel_idx;

        ledc_channel_config_t ledc_channel = {};
        ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
        ledc_channel.channel        = target_channel;
        ledc_channel.timer_sel      = LEDC_TIMER_0;
        ledc_channel.intr_type      = LEDC_INTR_DISABLE;
        ledc_channel.gpio_num       = pin;
        ledc_channel.duty           = 0;
        ledc_channel.hpoint         = 0;
        ledc_channel_config(&ledc_channel);

        // Update cache so subsequent calls skip this block
        pwm_cache[next_free_channel_idx].pin = pin;
        pwm_cache[next_free_channel_idx].ch = target_channel;
        pwm_cache[next_free_channel_idx].is_initialized = true;
        next_free_channel_idx++;
    }

    // 4. Update the duty cycle directly on the allocated channel
    // Cap duty at 255 to match the 8-bit timer resolution profile
    if (duty > 255) duty = 255;

    status |= ledc_set_duty(LEDC_LOW_SPEED_MODE, target_channel, duty);
    status |= ledc_update_duty(LEDC_LOW_SPEED_MODE, target_channel);
    return status;
}

void delayMicroseconds(uint32_t us) {
    esp_rom_delay_us(us);
}

void delay(uint32_t ms) {
    esp_rom_delay_us(ms*1000);
}

