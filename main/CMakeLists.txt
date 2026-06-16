cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES argos_hal argos_core argos_store argos_net argos_router
                    PRIV_REQUIRES esp_timer heap)
