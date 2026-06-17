cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/argos_net.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver freertos wifi_provisioning esp_http_server esp_timer argos_store argos_core)