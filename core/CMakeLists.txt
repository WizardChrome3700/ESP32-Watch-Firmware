idf_component_register(SRCS "./src/main.cpp"
                    INCLUDE_DIRS "./include"
                    REQUIRES freertos
                                esp_common
                                esp_hw_support
                                esp_timer
                                SPI
                                GPIO
                                Wire
                                LittleFS
                                Serial
                                spi_flash)