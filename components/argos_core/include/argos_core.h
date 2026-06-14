#ifndef ARGOS_CORE_H
#define ARGOS_CORE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Definiciones globales y tipos de datos para el framework Argos.
 */

/**
 * @brief Estructura para datos de medición.
 */
typedef struct {
    uint32_t timestamp;
    float value;
    uint8_t channel;
} argos_measurement_t;

/**
 * @brief Estructura para configuración global.
 */
typedef struct {
    uint32_t sample_rate;
    uint8_t adc_attenuation;
    uint8_t dac_resolution;
} argos_config_t;

/**
 * @brief Inicializa la configuración global de Argos.
 * @param config Puntero a la estructura de configuración.
 */
void argos_core_init(argos_config_t *config);

/**
 * @brief Crea una cola para el paso de mensajes entre tareas.
 * @param queue_size Tamaño de la cola.
 * @param item_size Tamaño de cada elemento en la cola.
 * @return Manejador de la cola creada.
 */
QueueHandle_t argos_core_create_queue(UBaseType_t queue_size, UBaseType_t item_size);

/**
 * @brief Crea un mutex para sincronización entre tareas.
 * @return Manejador del mutex creado.
 */
SemaphoreHandle_t argos_core_create_mutex(void);

#ifdef __cplusplus
}
#endif

#endif // ARGOS_CORE_H
