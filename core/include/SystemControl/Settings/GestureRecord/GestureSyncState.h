#pragma once
#ifndef GESTURESYNCSTATE_H
#define GESTURESYNCSTATE_H

#include "AppState.h"

class GestureSyncState: public AppState {
private:
    bool syncComplete;
public:
    GestureSyncState(AppContext* app_context) {
        this->app_context = app_context;
        this->syncComplete = false;
    }

    void onEnter() override {
        clearConsole();
        this->app_context->display->clearBuffer();
        this->app_context->display->clearBuffer();
        this->app_context->display->drawStringCentered(20, "USB SYNC", 1);
        this->app_context->display->drawStringCentered(40, "Waiting for PC...", 1);
        this->app_context->display->updateDisplay();

        // 1. Shift UART to High Speed (2 Mbps)
        Serial.flush();
        Serial.end();
        Serial.begin(2000000); 
    }

    void onProgress() override {
        if (syncComplete) return;

        if (Serial.available()) {
            uint8_t cmd = Serial.read();
            
            // 0xAA = PC requests synchronization
            if (cmd == 0xAA) {
                this->app_context->display->clearBuffer();
                this->app_context->display->drawStringCentered(30, "SYNCING...", 1);
                this->app_context->display->updateDisplay();

                File root = LittleFS.open("/", "r");
                if (root) {
                    File file = root.openNextFile();
                    while (file) {
                        const char* fileName = file.name();
                        
                        if (strstr(fileName, ".bin") != nullptr && 
                           (strncmp(fileName, "label", 5) == 0 || strncmp(fileName, "/label", 6) == 0)) {
                            
                            // ADD THIS: Destroy all stale 0xAA bytes in the buffer
                            while(Serial.available()) Serial.read();

                            // 1. Send Metadata (Text)
                            Serial.printf("FILE:%s:%d\r\n", fileName, file.size());
                            
                            // 2. Wait for PC Acknowledgment (0xBB) with a 2-second timeout
                            uint32_t wait_start = esp_timer_get_time() / 1000;
                            bool ack_received = false;
                            
                            while((esp_timer_get_time() / 1000) - wait_start < 2000) {
                                // ADD THIS: Drain the buffer instantly instead of 1 byte per 5ms
                                while(Serial.available()) {
                                    if (Serial.read() == 0xBB) {
                                        ack_received = true;
                                        break;
                                    }
                                }
                                if (ack_received) break;
                                vTaskDelay(pdMS_TO_TICKS(5));
                            }

                            // 3. Blast Binary Payload ONLY if ACK was received
                            if (ack_received) {
                                uint8_t chunk[1024];
                                int bytesRead;
                                while ((bytesRead = file.read(chunk, sizeof(chunk))) > 0) {
                                    Serial.writeBytes(chunk, bytesRead);
                                }
                            }
                        }
                        file = root.openNextFile();
                    }
                }
                
                // 4. Signal EOF to the PC script
                Serial.printf("DONE\r\n");

                syncComplete = true;
                this->app_context->display->clearBuffer();
                this->app_context->display->drawStringCentered(30, "SYNC COMPLETE", 1);
                this->app_context->display->updateDisplay();
            }
        }
    }

    AppState* handleInput(uint8_t buttonPressed) override {
        // Press CANCEL to abort sync and return to menu
        if(buttonPressed == 2) { 
            return nullptr; 
        }
        return this;
    }

    void onExit() override {
        // 3. Restore Standard UART Speed for OS logs
        Serial.flush();
        Serial.end();
        Serial.begin(115200); 

        this->app_context->display->clearBuffer();
        this->app_context->display->updateDisplay();
    }
};

#endif