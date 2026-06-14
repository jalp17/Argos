#include "argos_core.h"
#include "esp_log.h"

static const char *TAG = "argos_core";

void argos_core_init(argos_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Config pointer is NULL");
        return;
    }
    
    // Valores por defecto
    config->sample_rate = 1000; // 1 kHz
    config->adc_attenuation = 0; // 0 dB
    config->dac_resolution = 8; // 8 bits
    
    ESP_LOGI(TAG, "Argos core initialized");
}

QueueHandle_t argos_core_create_queue(UBaseType_t queue_size, UBaseType_t item_size) {
    QueueHandle_t queue = xQueueCreate(queue_size, item_size);
    if (queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
    }
    return queue;
}

SemaphoreHandle_t argos_core_create_mutex(void) {
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    if (mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
    }
    return mutex;
}
