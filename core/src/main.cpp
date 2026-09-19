// #include "main.h"

// static Main my_main;

// extern "C" void app_main(void)
// {
//     my_main.setup();
//     my_main.loop();
// }

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "SystemCtrl.h"
#include "ADS1256.h"

QueueHandle_t adc_data_queue = NULL;
volatile bool is_recording_gesture = false;
SemaphoreHandle_t spi_mutex = NULL;

// =========================================================================
// CORE 1 TASK: The ADC Polling Sandbox
// =========================================================================
void task_core1_adc(void *pvParameters) {
    // 1. Instantiate the ADC object
    ADS1256 adc(7, 15, 14, 10, ADS1256::DR_30000, ADS1256::GAIN_1);
    adc.init(false);

    AdcFrame current_frame;
    
    // 2. Add state trackers for the delta timer
    bool was_recording = false;
    uint32_t start_time_us = 0;

    while(true) {
        if (is_recording_gesture) {
            
            // 3. Capture the exact starting microsecond once per recording
            if (!was_recording) {
                start_time_us = (uint32_t)esp_timer_get_time();
                was_recording = true;
            }

            // 4. Stamp the frame with the elapsed delta time
            current_frame.timestamp = (uint32_t)esp_timer_get_time() - start_time_us;

            if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                // Sweep all 8 channels sequentially
                for(uint8_t i = 0; i < 8; i++) {
                    switch (i) {
                        case 0: current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0); break;
                        case 1: current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN1); break;
                        case 2: current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN2); break;
                        case 3: current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN3); break;
                        case 4: current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN4); break;
                        case 5: current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN5); break;
                        case 6: current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN6); break;
                        case 7: current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN7); break;
                        default: break;
                    }
                }
                xSemaphoreGive(spi_mutex);
            }
            
            // Send the 36-byte frame to the queue
            xQueueSend(adc_data_queue, &current_frame, 0);
            
            // Yield the CPU to allow the OLED screen to update
            taskYIELD();
            
        } else {
            // 5. Reset the state tracker when the recording stops
            was_recording = false;
            
            // If not recording, yield heavily so Core 1 can sleep
            vTaskDelay(pdMS_TO_TICKS(10)); 
        }
    }
}
// =========================================================================
// CORE 0 TASK: The System & UI Sandbox
// =========================================================================
void task_core0_system(void *pvParameters) {
    // 3. Instantiate the SystemCtrl object locally. 
    // It now lives exclusively in Core 0's task stack.
    SystemCtrl* sysctl = new SystemCtrl(15000);

    Serial.begin(115200);

    // 4. The Cold Boot Sequence (Runs strictly ONCE)
    sysctl->init();
    sysctl->boot_handler();

    // 5. The Permanent Execution Lifecycle
    while(true) {
        // Runs until screenTimeOut is reached
        sysctl->system_loop();
        
        // Drops into Light Sleep. CPU freezes here until woken by a button.
        sysctl->shutdown_handler();
    }
}

// =========================================================================
// THE KERNEL LAUNCHER
// =========================================================================
extern "C" void app_main() {
    spi_mutex = xSemaphoreCreateMutex(); // 2. Instantiate it
    // 6. Create the queue before launching the tasks
    adc_data_queue = xQueueCreate(100, sizeof(AdcFrame));

    // 7. Launch the Core 1 Task
    // Stack size is 8192 bytes. Ensure this is large enough to hold the ADS1256 object.
    xTaskCreatePinnedToCore(
        task_core1_adc, 
        "ADC_Task", 
        8192, 
        NULL, 
        5,      // High Priority
        NULL, 
        1       // Pinned to Core 1
    );

    // 8. Launch the Core 0 Task
    // Stack size is 8192 bytes. Ensure this is large enough to hold the SystemCtrl object.
    xTaskCreatePinnedToCore(
        task_core0_system, 
        "SysCtrl_Task", 
        8192, 
        NULL, 
        2,      // Lower Priority
        NULL, 
        0       // Pinned to Core 0
    );

    // 9. Destroy app_main. The two pinned tasks will now run forever.
    vTaskDelete(NULL); 
}