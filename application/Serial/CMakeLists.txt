idf_component_register(SRCS "Serial.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_uart
                                GPIO)