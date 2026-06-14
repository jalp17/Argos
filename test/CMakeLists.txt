cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_hal)

idf_component_register(SRCS "test_hal.c"
                    INCLUDE_DIRS "."
                    REQUIRES unity argos_hal argos_core
                    EMBED_FILES "")
