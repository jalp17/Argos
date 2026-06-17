cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/argos_core.c"
                    INCLUDE_DIRS "include"
                    REQUIRES freertos log)
