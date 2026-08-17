set(SOURCES SPI.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES esp_driver_spi
                                 GPIO)