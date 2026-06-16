#ifndef ARGOS_STORE_H
#define ARGOS_STORE_H

#include "argos_core.h"
#include "argos_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Componente de almacenamiento para Argos.
 * 
 * Proporciona LittleFS con buffer circular, log rotation y gestión de memoria crítica.
 * Soporta formatos CSV y JSON para exportación de datos.
 */

/* ==================== TYPE DEFINITIONS ==================== */

typedef enum {
    ARGOS_STORE_FORMAT_CSV = 0,
    ARGOS_STORE_FORMAT_JSON = 1
} argos_store_format_t;

typedef enum {
    ARGOS_STORE_OK = 0,
    ARGOS_STORE_ERR_NOT_INITIALIZED = -1,
    ARGOS_STORE_ERR_NO_SPACE = -2,
    ARGOS_STORE_ERR_CORRUPTED = -3,
    ARGOS_STORE_ERR_BUSY = -4,
    ARGOS_STORE_ERR_INVALID_ARG = -5
} argos_store_err_t;

typedef struct {
    size_t total_bytes;
    size_t used_bytes;
    size_t free_bytes;
    uint8_t usage_percent;
    bool critical;
} argos_store_stats_t;

typedef struct {
    char filename[64];
    size_t size;
    uint32_t timestamp;
    uint32_t record_count;
} argos_store_file_info_t;

typedef struct argos_store_handle_t argos_store_handle_t;

/* ==================== INITIALIZATION ==================== */

/**
 * @brief Inicializa el sistema de almacenamiento.
 * @return ESP_OK en éxito, código de error en fallo.
 */
esp_err_t argos_store_init(void);

/**
 * @brief Desinicializa el sistema de almacenamiento.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_deinit(void);

/**
 * @brief Verifica si el almacenamiento está inicializado.
 * @return true si está listo.
 */
bool argos_store_is_ready(void);

/* ==================== DATA LOGGING ==================== */

/**
 * @brief Escribe una medición al buffer circular (no bloqueante).
 * @param measurement Puntero a la medición.
 * @return ESP_OK si se encoló, ARGOS_STORE_ERR_BUSY si buffer lleno.
 */
esp_err_t argos_store_write_measurement(const argos_measurement_t *measurement);

/**
 * @brief Escribe múltiples mediciones al buffer.
 * @param measurements Array de mediciones.
 * @param count Número de mediciones.
 * @return Número de mediciones escritas.
 */
size_t argos_store_write_measurements(const argos_measurement_t *measurements, size_t count);

/**
 * @brief Fuerza flush del buffer a flash.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_flush(void);

/* ==================== FILE MANAGEMENT ==================== */

/**
 * @brief Crea un nuevo archivo de log con timestamp.
 * @param format Formato de salida (CSV/JSON).
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_create_log_file(argos_store_format_t format);

/**
 * @brief Cierra el archivo de log actual.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_close_log_file(void);

/**
 * @brief Lista archivos de log disponibles.
 * @param files Array para almacenar información.
 * @param max_files Tamaño máximo del array.
 * @param count Puntero para número de archivos encontrados.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_list_files(argos_store_file_info_t *files, size_t max_files, size_t *count);

/**
 * @brief Elimina el archivo de log más antiguo.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_delete_oldest(void);

/* ==================== DATA EXPORT ==================== */

/**
 * @brief Exporta un archivo de log a buffer para descarga web.
 * @param filename Nombre del archivo.
 * @param buffer Buffer de destino.
 * @param buffer_size Tamaño del buffer.
 * @param bytes_read Puntero para bytes leídos.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_export_file(const char *filename, uint8_t *buffer, size_t buffer_size, size_t *bytes_read);

/**
 * @brief Obtiene estadísticas de uso del almacenamiento.
 * @param stats Estructura para estadísticas.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_get_stats(argos_store_stats_t *stats);

/* ==================== LOG ROTATION ==================== */

/**
 * @brief Verifica y ejecuta rotación de logs si es necesario.
 * @return ESP_OK en éxito, ARGOS_STORE_ERR_NO_SPACE si crítico.
 */
esp_err_t argos_store_check_rotation(void);

/**
 * @brief Configura umbrales de rotación.
 * @param warning_pct Porcentaje de advertencia (default 85%).
 * @param critical_pct Porcentaje crítico (default 95%).
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_set_thresholds(uint8_t warning_pct, uint8_t critical_pct);

/* ==================== DEBUG & DIAGNOSTICS ==================== */

/**
 * @brief Imprime diagnóstico del almacenamiento.
 */
void argos_store_print_diagnostics(void);

/**
 * @brief Realiza auto-test del almacenamiento.
 * @return ESP_OK si todo funciona.
 */
esp_err_t argos_store_self_test(void);

/**
 * @brief Formatea la partición de almacenamiento (¡CUIDADO: borra todo!).
 * @return ESP_OK en éxito.
 */
esp_err_t argos_store_format(void);

/* ==================== CALLBACKS ==================== */

/**
 * @brief Tipo de callback para notificaciones de almacenamiento.
 */
typedef void (*argos_store_event_cb_t)(int event, void *arg);

/**
 * @brief Registra callback para eventos de almacenamiento.
 * Eventos: 0=warning space, 1=critical space, 2=rotation done, 3=flush done
 * @param cb Callback function.
 * @param arg Argumento de usuario.
 */
void argos_store_register_callback(argos_store_event_cb_t cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif // ARGOS_STORE_H
