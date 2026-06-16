cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "test_hal.c" "test_store.c" "test_net.c"
                    INCLUDE_DIRS "."
                    REQUIRES unity argos_hal argos_core argos_store argos_net
                    EMBED_FILES "")