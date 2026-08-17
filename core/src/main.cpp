// #include "main.h"

// static Main my_main;

// extern "C" void app_main(void)
// {
//     my_main.setup();
//     my_main.loop();
// }

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "SystemCtrl.h"
#include "ADS1256.h"

QueueHandle_t adc_data_queue = NULL;
volatile bool is_recording_gesture = false;

// =========================================================================
// CORE 1 TASK: The ADC Polling Sandbox
// =========================================================================
void task_core1_adc(void *pvParameters) {
    // 2. Instantiate the ADC object locally. 
    // It now lives exclusively in Core 1's task stack.
    ADS1256 adc(7, 15, 14, 10, ADS1256::DR_1000, ADS1256::GAIN_1);
    
    // Initialize hardware from Core 1
    adc.init(false);

    AdcFrame current_frame;

    while(true) {
        // Only burn CPU cycles reading the SPI bus if we actually need the data
        if (is_recording_gesture) {
            
            // Sweep all 8 channels (ensure your ADS1256 driver multiplexes correctly)
            for(uint8_t i = 0; i < 8; i++) {
                switch (i)
                {
                case 1:
                    current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0);
                    break;
                case 2:
                    current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0);
                    break;
                case 3:
                    current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0);
                    break;
                case 4:
                    current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0);
                    break;
                case 5:
                    current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0);
                    break;
                case 6:
                    current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0);
                    break;
                case 7:
                    current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0);
                    break;
                case 8:
                    current_frame.channels[i] = adc.readSingleEnded(ADS1256::AIN0);
                    break;
                default:
                    break;
                }
            }
            
            // Shove the complete 8-channel frame into the queue.
            // Timeout is 0. If the queue is full (Core 0 is too slow writing to flash), 
            // we intentionally drop the frame to keep Core 1 running at top speed.
            xQueueSend(adc_data_queue, &current_frame, 0);
            // ADD THIS: Force Core 1 to yield for 1 RTOS tick (1ms) after every frame.
            // This gives Core 0's OLED the tiny window it needs to slip in and grab the SPI mutex!
            vTaskDelay(pdMS_TO_TICKS(1));
        } else {
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
    SystemCtrl sysctl(15000);

    // 4. The Cold Boot Sequence (Runs strictly ONCE)
    sysctl.init();
    sysctl.boot_handler();

    // 5. The Permanent Execution Lifecycle
    while(true) {
        // Runs until screenTimeOut is reached
        sysctl.system_loop();
        
        // Drops into Light Sleep. CPU freezes here until woken by a button.
        sysctl.shutdown_handler();
    }
}

// =========================================================================
// THE KERNEL LAUNCHER
// =========================================================================
extern "C" void app_main() {
    // 6. Create the queue before launching the tasks
    adc_data_queue = xQueueCreate(20, sizeof(AdcFrame));

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