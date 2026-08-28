#include "AppState.h"

extern QueueHandle_t adc_data_queue;
extern volatile bool is_recording_gesture;

struct AdcFrame {
    int32_t channels[8];
};

class GestureRecordState: public AppState {
private:
    File recordFile;
    uint8_t* pageBuffer;        // 4KB Raw Byte Buffer in PSRAM
    uint16_t bufferOffset;      // Tracks byte position in the 4KB buffer
    uint32_t recordStartTime;   
    char currentLabel[10];
    
    bool isFirstFrame;          // Delta compression flag
    AdcFrame previousFrame;     // Holds S_{n-1} for delta calculation
public:
    GestureRecordState(AppContext* app_context, const char* label) {
        this->app_context = app_context;
        strcpy(this->currentLabel, label);
        this->bufferOffset = 0;
        this->pageBuffer = nullptr;
        this->isFirstFrame = true;
    }
    void onEnter() override {
        clearConsole();
        
        // 1. Allocate a flat 4,096-byte page buffer in PSRAM
        pageBuffer = (uint8_t*) heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
        
        if (pageBuffer == nullptr) {
            ESP_LOGE("GESTURE", "PSRAM allocation failed!");
            return;
        }

        // 2. Open LittleFS File
        char filename[32];
        // sprintf(filename, "/%s_%lu.bin", currentLabel, convertDate2Epoch(this->app_context->currentTime));
        sprintf(filename, "/%s_%llu.bin", currentLabel, (uint64_t)esp_timer_get_time() / 1000000);
        recordFile = LittleFS.open(filename, "w");

        // 3. Update OLED
        this->app_context->display->clearBuffer();
        this->app_context->display->drawStringCentered(20, "RECORDING...", 1);
        this->app_context->display->drawStringCentered(40, currentLabel, 1);
        this->app_context->display->updateDisplay();

        // 4. Start Capture
        recordStartTime = esp_timer_get_time() / 1000;
        is_recording_gesture = true; 
    }
    void onProgress() override {
        if(!recordFile || pageBuffer == nullptr) return;

        AdcFrame incomingFrame;

        // Pull from Queue
        if (xQueueReceive(adc_data_queue, &incomingFrame, 0) == pdPASS) {
            
            if (isFirstFrame) {
                // WRITE ABSOLUTE (32 Bytes)
                // Check if buffer has room (it always will on first frame)
                memcpy(&pageBuffer[bufferOffset], &incomingFrame, sizeof(AdcFrame));
                bufferOffset += sizeof(AdcFrame); // 32
                
                previousFrame = incomingFrame;
                isFirstFrame = false;
            } 
            else {
                // WRITE DELTA (16 Bytes)
                int16_t deltas[8];
                for(uint8_t i = 0; i < 8; i++) {
                    // Calculate S_n - S_{n-1}
                    deltas[i] = (int16_t)(incomingFrame.channels[i] - previousFrame.channels[i]);
                }

                // Check if we have 16 bytes of space left in the 4KB buffer
                if (bufferOffset + sizeof(deltas) > 4096) {
                    recordFile.write(pageBuffer, bufferOffset);
                    bufferOffset = 0; // Reset
                }

                memcpy(&pageBuffer[bufferOffset], deltas, sizeof(deltas));
                bufferOffset += sizeof(deltas); // 16
                
                previousFrame = incomingFrame;
            }
        }

        // 3-Second Timeout Check
        if ((esp_timer_get_time() / 1000) - recordStartTime >= 3000) {
            this->handleInput(2); // Simulate CANCEL to exit
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
        
        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
    AppState* handleInput(uint8_t buttonPressed) override;
};