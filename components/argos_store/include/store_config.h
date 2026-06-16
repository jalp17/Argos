#ifndef STORE_CONFIG_H
#define STORE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuración de almacenamiento para Argos.
 * 
 * Define particiones, límites y políticas de rotación de logs.
 */

/* ==================== PARTITION CONFIGURATION ==================== */
#define ARGOS_STORE_PARTITION_LABEL   "storage"
#define ARGOS_STORE_BASE_PATH         "/data"
#define ARGOS_STORE_MAX_FILES         10
#define ARGOS_STORE_MAX_FILE_SIZE     (512 * 1024)  // 512 KB per file

/* ==================== CIRCULAR BUFFER CONFIGURATION ==================== */
#define ARGOS_STORE_BUFFER_SIZE       (64 * 1024)   // 64 KB RAM buffer
#define ARGOS_STORE_FLUSH_INTERVAL_MS 1000          // Flush to flash every 1s
#define ARGOS_STORE_FLUSH_THRESHOLD   (ARGOS_STORE_BUFFER_SIZE * 3 / 4)  // 75% threshold

/* ==================== LOG ROTATION CONFIGURATION ==================== */
#define ARGOS_STORE_ROTATION_THRESHOLD_PCT  85  // Rotate at 85% partition usage
#define ARGOS_STORE_CRITICAL_THRESHOLD_PCT  95  // Stop writing at 95%
#define ARGOS_STORE_MAX_LOG_FILES           20  // Max log files before rotation
#define ARGOS_STORE_LOG_PREFIX              "argos_log"
#define ARGOS_STORE_LOG_EXTENSION           ".csv"

/* ==================== DATA FORMAT CONFIGURATION ==================== */
#define ARGOS_STORE_TIMESTAMP_FORMAT  "%Y-%m-%d %H:%M:%S"
#define ARGOS_STORE_CSV_HEADER        "timestamp,channel,value_mv,raw\n"
#define ARGOS_STORE_JSON_INDENT       2

/* ==================== WEAR LEVELING ==================== */
#define ARGOS_STORE_WEAR_LEVELING     1  // Enable LittleFS wear leveling

/* ==================== DEBUG ==================== */
#define ARGOS_STORE_DEBUG_ENABLE      1
#define ARGOS_STORE_DEBUG_TAG         "argos_store"

#ifdef __cplusplus
}
#endif

#endif // STORE_CONFIG_H
