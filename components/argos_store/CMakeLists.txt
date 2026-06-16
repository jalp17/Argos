cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/argos_store.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver freertos esp_littlefs vfs)
