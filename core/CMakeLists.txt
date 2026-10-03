# idf_component_register(SRCS "./src/main.cpp"
#                     INCLUDE_DIRS "./include"
#                     REQUIRES freertos
#                                 esp_common
#                                 esp_hw_support
#                                 esp_timer
#                                 SPI
#                                 GPIO
#                                 Wire
#                                 LittleFS
#                                 Serial
#                                 spi_flash)
idf_component_register(SRCS "./src/main.cpp"
                    INCLUDE_DIRS "./include"
                                 "./include/ADC"
                                 "./include/Calendar"
                                 "./include/Display"
                                 "./include/RTC"
                                 "./include/SystemControl"
                                 "./include/SystemControl/Calendar"
                                 "./include/SystemControl/HomeState"
                                 "./include/SystemControl/Settings"
                                 "./include/SystemControl/Settings/GestureRecord"
                                 "./include/SystemControl/Settings/WifiState"
                                 "./include/Time"
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