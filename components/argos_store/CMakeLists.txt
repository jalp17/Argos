cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/argos_store.c"
                    INCLUDE_DIRS "include"
                     REQUIRES argos_core argos_hal driver freertos spiffs vfs)
