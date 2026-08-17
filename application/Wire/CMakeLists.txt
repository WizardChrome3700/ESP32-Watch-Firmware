# idf_component_register(
#     SRCS
#         "Wire.cpp"
#         "main.cpp"
#     INCLUDE_DIRS "."
#     REQUIRES
#         esp_driver_gpio
#         esp_driver_i2c
#         esp_adc
#         esp_timer
#         freertos
#         driver
#         log

# )

set(SOURCES Wire.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES esp_driver_i2c
                                 GPIO
                                 log
                                 freertos)
    