cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_argos)

idf_component_register(SRCS "test_hal.c" "test_store.c"
                    INCLUDE_DIRS "."
                    REQUIRES unity argos_hal argos_core argos_store
                    EMBED_FILES "")
