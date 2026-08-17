set(SOURCES GPIO.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES esp_driver_gpio
                                 esp_driver_ledc)