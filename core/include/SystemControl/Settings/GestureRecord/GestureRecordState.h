#include "AppState.h"

extern QueueHandle_t adc_data_queue;
extern volatile bool is_recording_gesture;

/**
 * @brief A struct containing the sensor readings of 8 sensor obtained from ADS1256 ADC.
 * 
 * It is used as a datastructure to store and manipulate sensor readings obtain from ADS1256 ADC.
 */
struct AdcFrame {
    uint32_t timestamp;  // ADD THIS: Delta time in microseconds
    int32_t channels[8];
};

class GestureRecordState: public AppState {
private:
    File recordFile;
    uint8_t* pageBuffer;        // 4KB Raw Byte Buffer in PSRAM
    uint16_t bufferOffset;      // Tracks byte position in the 4KB buffer
    uint32_t recordStartTime;   
    char currentLabel[10];
public:
    GestureRecordState(AppContext* app_context, const char* label) {
        this->app_context = app_context;
        strcpy(this->currentLabel, label);
        this->bufferOffset = 0;
        this->pageBuffer = nullptr;
    }
    void onEnter() override {
        uint32_t t_init = esp_timer_get_time();
        uint32_t t_prev = t_init;
        clearConsole();
        this->app_context->display->clearBuffer();
        // 1. Allocate a flat 4,096-byte page buffer in PSRAM
        pageBuffer = (uint8_t*) heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
        
        if (pageBuffer == nullptr) {
            ESP_LOGE("GESTURE", "PSRAM allocation failed!");
            return;
        }

        // 2. Open LittleFS File
        char filename[32];
        sprintf(filename, "/%s_%lu.bin", currentLabel, convertDate2Epoch(this->app_context->currentTime));
        sprintf(filename, "/%s_%llu.bin", currentLabel, (uint64_t)esp_timer_get_time() / 1000000);
        recordFile = LittleFS.open(filename, "w");

        // 3. Update OLED
        this->app_context->display->clearBuffer();
        this->app_context->display->drawStringCentered(20, "RECORDING...", 1);
        this->app_context->display->drawStringCentered(32, currentLabel, 1);
        this->app_context->display->updateDisplay();

        // 4. Start Capture
        recordStartTime = esp_timer_get_time() / 1000;
        is_recording_gesture = true; 
    }
    void onProgress() override {
        if(!recordFile || pageBuffer == nullptr) return;

        AdcFrame incomingFrame;

        // Pull from Queue until it is completely empty
        while (xQueueReceive(adc_data_queue, &incomingFrame, 0) == pdPASS) {
            
            if (bufferOffset + sizeof(AdcFrame) > 4096) {
                if (recordFile) {
                    recordFile.write(pageBuffer, bufferOffset);
                }
                bufferOffset = 0; // Reset
            }
            memcpy(&pageBuffer[bufferOffset], &incomingFrame, sizeof(AdcFrame));
            bufferOffset += sizeof(AdcFrame); // 32
        }

        // 3-Second Timeout Check
        if ((esp_timer_get_time() / 1000) - recordStartTime >= 3000) {
            // this->handleInput(2); // Simulate CANCEL to exit
            this->app_context->display->fillRect(1, 20, 127, 10, 0);
            this->app_context->display->drawStringCentered(20, "RECORDING DONE", 1);
            this->app_context->display->drawStringCentered(32, currentLabel, 1);
            this->app_context->display->updateDisplay();
            this->requestExit = true;   
        }
        else {
            float elapsed = (float)(esp_timer_get_time() / 1000) - (float)recordStartTime;
            // Prevent negative drawing coordinates
            if (elapsed < 0) elapsed = 0; 
            float progressBarLength = (elapsed / 3000.0) * 127.0;
            // Serial.printf("Progress bar: %f\r\n", progressBarLength);
            // this->app_context->display->drawLine(1, 62, 127, 62, 0);
            // this->app_context->display->drawLine(1, 62, (uint8_t)progressBarLength, 62, 1);
            // this->app_context->display->updateDisplay();
        }
    }
    void onExit() override {
        is_recording_gesture = false;

        // Flush remaining bytes
        if (recordFile && bufferOffset > 0) {
            recordFile.write(pageBuffer, bufferOffset);
        }

        if(recordFile) recordFile.close();
        if(pageBuffer != nullptr) heap_caps_free(pageBuffer);
        xQueueReset(adc_data_queue);

        Serial.println("\r\n--- LittleFS File List ---");
        File root = LittleFS.open("/", "r");
        if (root) {
            File file = root.openNextFile();
            while (file) {
                Serial.printf("File: %s | Size: %d bytes\r\n", file.name(), file.size());
                file = root.openNextFile();
            }
        } else {
            Serial.println("Failed to open root directory.");
        }
        Serial.println("--------------------------\r\n");
        
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
        
    }
    AppState* handleInput(uint8_t buttonPressed) override;
};