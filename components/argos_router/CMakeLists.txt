cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/argos_router.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver freertos argos_hal argos_store argos_net)