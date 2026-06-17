cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/argos_hal.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_adc freertos
                    PRIV_REQUIRES)
