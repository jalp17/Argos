#ifndef ARGOS_ROUTER_H
#define ARGOS_ROUTER_H

#include "argos_core.h"
#include "argos_hal.h"
#include "experiment_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enrutador de datos y control de experimentos para Argos.
 *
 * Gestiona colas FreeRTOS para el ruteo de mediciones entre HAL, Store y WebSocket.
 * Controla el inicio/parada de experimentos y aplica algoritmos de práctica.
 */

/* ==================== CONSTANTES ==================== */
#define ARGOS_ROUTER_QUEUE_LENGTH    64
#define ARGOS_ROUTER_NUM_DESTINOS    3

typedef enum {
    ARGOS_ROUTER_DEST_SERIAL = 0,
    ARGOS_ROUTER_DEST_STORE = 1,
    ARGOS_ROUTER_DEST_WEBSOCKET = 2
} argos_router_dest_t;

/* ==================== INICIALIZACIÓN ==================== */

/**
 * @brief Inicializa el enrutador con una configuración de experimento.
 * @param config Puntero a la configuración del experimento.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_init(const argos_experiment_config_t *config);

/**
 * @brief Desinicializa el enrutador y libera recursos.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_deinit(void);

/**
 * @brief Verifica si el enrutador está listo.
 */
bool argos_router_is_ready(void);

/* ==================== CONTROL DE EXPERIMENTO ==================== */

/**
 * @brief Inicia la captura de datos según la configuración.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_start_experiment(void);

/**
 * @brief Detiene la captura de datos.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_stop_experiment(void);

/**
 * @brief Pausa/reanuda la captura.
 * @param pausar true para pausar, false para reanudar.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_pause_experiment(bool pausar);

/**
 * @brief Obtiene el estado actual del experimento.
 * @return Estado actual.
 */
exp_state_t argos_router_get_state(void);

/* ==================== GESTIÓN DE CONFIGURACIÓN ==================== */

/**
 * @brief Obtiene la configuración actual del experimento.
 * @return Puntero a la configuración (no modificar directamente).
 */
const argos_experiment_config_t *argos_router_get_config(void);

/**
 * @brief Actualiza la configuración del experimento (solo en estado detenido).
 * @param config Nueva configuración.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_set_config(const argos_experiment_config_t *config);

/**
 * @brief Carga una plantilla de configuración desde archivo.
 * @param nombre Nombre de la plantilla.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_load_template(const char *nombre);

/**
 * @brief Guarda la configuración actual como plantilla.
 * @param nombre Nombre para la plantilla.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_save_template(const char *nombre);

/* ==================== ENRUTAMIENTO DE DATOS ==================== */

/**
 * @brief Enruta una medición a todos los destinos configurados.
 * @param medicion Puntero a la medición.
 * @return Número de destinos a los que se entregó.
 */
int argos_router_route_measurement(const argos_measurement_t *medicion);

/**
 * @brief Enruta una medición a un destino específico.
 * @param medicion Puntero a la medición.
 * @param destino Destino de enrutamiento.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_route_to(const argos_measurement_t *medicion, argos_router_dest_t destino);

/* ==================== ALGORITMOS DE PRÁCTICA ==================== */

/**
 * @brief Ejecuta un paso del algoritmo configurado.
 * @param entrada Valor de entrada (ADC).
 * @param salida Puntero para el valor de salida (DAC/PWM).
 * @return ESP_OK si el algoritmo continúa, ESP_FAIL si terminó.
 */
esp_err_t argos_router_run_algorithm_step(float entrada, float *salida);

/**
 * @brief Obtiene la cola de datos para que las tareas consuman.
 * @return Manejador de la cola.
 */
QueueHandle_t argos_router_get_data_queue(void);

/**
 * @brief Obtiene la cola de control para comandos.
 * @return Manejador de la cola.
 */
QueueHandle_t argos_router_get_control_queue(void);

/* ==================== TAREA DE ADQUISICIÓN ==================== */

/**
 * @brief Inicia la tarea de adquisición continua.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_start_acquisition_task(void);

/**
 * @brief Tarea interna de adquisición (no llamar directamente).
 * @param arg Argumento (no usado).
 */
void argos_router_acquisition_task(void *arg);

/* ==================== LOGGING MULTICANAL ==================== */

/**
 * @brief Envía un mensaje de log a todos los canales.
 * @param level Nivel de log (ESP_LOG_INFO, etc.)
 * @param tag Etiqueta.
 * @param format Formato y argumentos.
 */
void argos_router_log(int level, const char *tag, const char *format, ...);

/* ==================== ESTADÍSTICAS Y DEBUG ==================== */

typedef struct {
    uint32_t total_mediciones;
    uint32_t mediciones_segundo;
    uint32_t errores;
    uint32_t bytes_almacenados;
    uint32_t tiempo_activo_seg;
    uint8_t uso_buffer_pct;
} argos_router_stats_t;

/**
 * @brief Obtiene estadísticas del enrutador.
 * @param stats Puntero a estadísticas.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_get_stats(argos_router_stats_t *stats);

/**
 * @brief Imprime diagnóstico completo del enrutador.
 */
void argos_router_print_diagnostics(void);

/**
 * @brief Realiza auto-test del enrutador.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_router_self_test(void);

#ifdef __cplusplus
}
#endif

#endif // ARGOS_ROUTER_H
