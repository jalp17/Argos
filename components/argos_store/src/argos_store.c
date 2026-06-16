#include "argos_store.h"
#include "store_config.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_littlefs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

static const char *TAG = ARGOS_STORE_DEBUG_TAG;

/* ==================== DEBUG MACROS ==================== */
#if ARGOS_STORE_DEBUG_ENABLE
#define STORE_LOG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define STORE_ERR(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define STORE_WARN(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
#define STORE_LOG(fmt, ...)
#define STORE_ERR(fmt, ...)
#define STORE_WARN(fmt, ...)
#endif

/* ==================== CIRCULAR BUFFER ==================== */
typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    SemaphoreHandle_t mutex;
} circular_buffer_t;

/* ==================== INTERNAL STATE ==================== */
static bool s_initialized = false;
static circular_buffer_t s_circular_buf;
static SemaphoreHandle_t s_file_mutex = NULL;
static SemaphoreHandle_t s_flush_sem = NULL;
static TaskHandle_t s_flush_task_handle = NULL;
static char s_current_log_file[64] = {0};
static uint8_t s_critical_threshold = ARGOS_STORE_CRITICAL_THRESHOLD_PCT;
static uint8_t s_warning_threshold = ARGOS_STORE_ROTATION_THRESHOLD_PCT;
static bool s_critical_state = false;
static argos_store_event_cb_t s_event_cb = NULL;
static void *s_event_cb_arg = NULL;

/* ==================== CIRCULAR BUFFER FUNCTIONS ==================== */

static esp_err_t circular_buffer_init(circular_buffer_t *cb, size_t size) {
    cb->buffer = (uint8_t *)calloc(1, size);
    if (cb->buffer == NULL) {
        STORE_ERR("Failed to allocate circular buffer (%d bytes)", size);
        return ESP_ERR_NO_MEM;
    }
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    cb->mutex = xSemaphoreCreateMutex();
    if (cb->mutex == NULL) {
        free(cb->buffer);
        return ESP_ERR_NO_MEM;
    }
    STORE_LOG("Circular buffer initialized: %d bytes", size);
    return ESP_OK;
}

static void circular_buffer_deinit(circular_buffer_t *cb) {
    if (cb->buffer != NULL) {
        free(cb->buffer);
        cb->buffer = NULL;
    }
    if (cb->mutex != NULL) {
        vSemaphoreDelete(cb->mutex);
        cb->mutex = NULL;
    }
    cb->size = 0;
    cb->count = 0;
}

static esp_err_t circular_buffer_push(circular_buffer_t *cb, const uint8_t *data, size_t len) {
    if (xSemaphoreTake(cb->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (cb->count + len > cb->size) {
        xSemaphoreGive(cb->mutex);
        return ESP_ERR_NO_MEM;
    }

    for (size_t i = 0; i < len; i++) {
        cb->buffer[cb->head] = data[i];
        cb->head = (cb->head + 1) % cb->size;
        cb->count++;
    }

    xSemaphoreGive(cb->mutex);
    return ESP_OK;
}

static size_t circular_buffer_pop(circular_buffer_t *cb, uint8_t *data, size_t max_len) {
    if (xSemaphoreTake(cb->mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }

    size_t read_count = 0;
    while (cb->count > 0 && read_count < max_len) {
        data[read_count] = cb->buffer[cb->tail];
        cb->tail = (cb->tail + 1) % cb->size;
        cb->count--;
        read_count++;
    }

    xSemaphoreGive(cb->mutex);
    return read_count;
}

static size_t circular_buffer_available(circular_buffer_t *cb) {
    size_t count;
    if (xSemaphoreTake(cb->mutex, portMAX_DELAY) == pdTRUE) {
        count = cb->count;
        xSemaphoreGive(cb->mutex);
    } else {
        count = 0;
    }
    return count;
}

/* ==================== FLUSH TASK ==================== */

static void flush_task(void *arg) {
    uint8_t *temp_buffer = (uint8_t *)malloc(ARGOS_STORE_BUFFER_SIZE);
    if (temp_buffer == NULL) {
        STORE_ERR("Failed to allocate flush temp buffer");
        vTaskDelete(NULL);
        return;
    }

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(ARGOS_STORE_FLUSH_INTERVAL_MS));

        size_t avail = circular_buffer_available(&s_circular_buf);
        if (avail == 0) continue;

        size_t flush_threshold = (ARGOS_STORE_BUFFER_SIZE > ARGOS_STORE_FLUSH_THRESHOLD)
                                  ? ARGOS_STORE_FLUSH_THRESHOLD : ARGOS_STORE_BUFFER_SIZE;

        if (avail >= flush_threshold || avail > 0) {
            size_t to_read = (avail < ARGOS_STORE_BUFFER_SIZE) ? avail : ARGOS_STORE_BUFFER_SIZE;
            size_t read = circular_buffer_pop(&s_circular_buf, temp_buffer, to_read);
            if (read > 0) {
                if (xSemaphoreTake(s_file_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    if (s_current_log_file[0] != '\0') {
                        FILE *f = fopen(s_current_log_file, "a");
                        if (f != NULL) {
                            fwrite(temp_buffer, 1, read, f);
                            fclose(f);
                        }
                    }
                    xSemaphoreGive(s_file_mutex);
                }
                if (s_event_cb) s_event_cb(3, s_event_cb_arg);
            }
        }
    }

    free(temp_buffer);
}

/* ==================== PARTITION MANAGEMENT ==================== */

static esp_err_t get_partition_stats(argos_store_stats_t *stats) {
    if (stats == NULL) return ESP_ERR_INVALID_ARG;

    size_t total = 0, used = 0;
    esp_err_t ret = esp_littlefs_info(ARGOS_STORE_PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK) {
        STORE_ERR("Failed to get partition info: %s", esp_err_to_name(ret));
        return ret;
    }

    stats->total_bytes = total;
    stats->used_bytes = used;
    stats->free_bytes = total - used;
    stats->usage_percent = (total > 0) ? (uint8_t)((used * 100) / total) : 0;
    stats->critical = (stats->usage_percent >= s_critical_threshold);

    return ESP_OK;
}

/* ==================== PUBLIC API ==================== */

esp_err_t argos_store_init(void) {
    if (s_initialized) {
        STORE_WARN("Store already initialized");
        return ESP_OK;
    }

    STORE_LOG("Initializing storage...");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = ARGOS_STORE_BASE_PATH,
        .partition_label = ARGOS_STORE_PARTITION_LABEL,
        .format_if_mount_failed = true,
        .max_files = ARGOS_STORE_MAX_FILES,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        STORE_ERR("Failed to mount LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = circular_buffer_init(&s_circular_buf, ARGOS_STORE_BUFFER_SIZE);
    if (ret != ESP_OK) {
        esp_vfs_littlefs_unregister(ARGOS_STORE_PARTITION_LABEL);
        return ret;
    }

    s_file_mutex = xSemaphoreCreateMutex();
    if (s_file_mutex == NULL) {
        circular_buffer_deinit(&s_circular_buf);
        esp_vfs_littlefs_unregister(ARGOS_STORE_PARTITION_LABEL);
        return ESP_ERR_NO_MEM;
    }

    ret = argos_store_create_log_file(ARGOS_STORE_FORMAT_CSV);
    if (ret != ESP_OK) {
        STORE_WARN("Failed to create initial log file");
    }

    BaseType_t task_created = xTaskCreatePinnedToCore(
        flush_task, "store_flush", 4096, NULL, 5, &s_flush_task_handle, tskNO_AFFINITY);
    if (task_created != pdPASS) {
        STORE_ERR("Failed to create flush task");
        vSemaphoreDelete(s_file_mutex);
        circular_buffer_deinit(&s_circular_buf);
        esp_vfs_littlefs_unregister(ARGOS_STORE_PARTITION_LABEL);
        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;
    STORE_LOG("Storage initialized successfully");

    argos_store_stats_t stats;
    if (argos_store_get_stats(&stats) == ESP_OK) {
        STORE_LOG("Partition: %d KB total, %d KB used (%d%%)",
                  stats.total_bytes / 1024, stats.used_bytes / 1024, stats.usage_percent);
        if (stats.usage_percent >= s_warning_threshold) {
            STORE_WARN("Partition usage %d%% exceeds warning threshold", stats.usage_percent);
            if (s_event_cb) s_event_cb(0, s_event_cb_arg);
        }
        if (stats.critical) {
            STORE_ERR("Partition usage %d%% is CRITICAL!", stats.usage_percent);
            s_critical_state = true;
            if (s_event_cb) s_event_cb(1, s_event_cb_arg);
        }
    }

    return ESP_OK;
}

esp_err_t argos_store_deinit(void) {
    if (!s_initialized) return ESP_OK;

    STORE_LOG("Deinitializing storage...");

    argos_store_flush();

    if (s_flush_task_handle != NULL) {
        vTaskDelete(s_flush_task_handle);
        s_flush_task_handle = NULL;
    }

    if (s_current_log_file[0] != '\0') {
        argos_store_close_log_file();
    }

    circular_buffer_deinit(&s_circular_buf);

    if (s_file_mutex != NULL) {
        vSemaphoreDelete(s_file_mutex);
        s_file_mutex = NULL;
    }

    esp_vfs_littlefs_unregister(ARGOS_STORE_PARTITION_LABEL);

    s_initialized = false;
    STORE_LOG("Storage deinitialized");
    return ESP_OK;
}

bool argos_store_is_ready(void) {
    return s_initialized && !s_critical_state;
}

esp_err_t argos_store_write_measurement(const argos_measurement_t *measurement) {
    if (!s_initialized || measurement == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_critical_state) {
        return ESP_ERR_NO_MEM;
    }

    char line[128];
    int len = snprintf(line, sizeof(line), "%lu,%d,%d,%.2f\n",
                       measurement->timestamp,
                       measurement->channel,
                       (int)(measurement->value * 1000),
                       (int)measurement->value);

    size_t available = circular_buffer_available(&s_circular_buf);
    if (available + len > s_circular_buf.size) {
        STORE_ERR("Circular buffer full!");
        return ESP_ERR_NO_MEM;
    }

    return circular_buffer_push(&s_circular_buf, (uint8_t *)line, len);
}

size_t argos_store_write_measurements(const argos_measurement_t *measurements, size_t count) {
    if (!s_initialized || measurements == NULL) return 0;

    size_t written = 0;
    for (size_t i = 0; i < count; i++) {
        if (argos_store_write_measurement(&measurements[i]) == ESP_OK) {
            written++;
        } else {
            break;
        }
    }
    return written;
}

esp_err_t argos_store_flush(void) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    size_t avail = circular_buffer_available(&s_circular_buf);
    if (avail == 0) return ESP_OK;

    uint8_t *temp = (uint8_t *)malloc(avail);
    if (temp == NULL) return ESP_ERR_NO_MEM;

    size_t read = circular_buffer_pop(&s_circular_buf, temp, avail);
    if (read > 0 && s_current_log_file[0] != '\0') {
        FILE *f = fopen(s_current_log_file, "a");
        if (f != NULL) {
            fwrite(temp, 1, read, f);
            fclose(f);
        }
    }

    free(temp);
    return ESP_OK;
}

esp_err_t argos_store_create_log_file(argos_store_format_t format) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(s_file_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    const char *ext = (format == ARGOS_STORE_FORMAT_JSON) ? ".json" : ".csv";
    snprintf(s_current_log_file, sizeof(s_current_log_file),
             "%s/%s_%04d%02d%02d_%02d%02d%02d%s",
             ARGOS_STORE_BASE_PATH, ARGOS_STORE_LOG_PREFIX,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ext);

    FILE *f = fopen(s_current_log_file, "w");
    if (f == NULL) {
        STORE_ERR("Failed to create log file: %s", s_current_log_file);
        xSemaphoreGive(s_file_mutex);
        return ESP_FAIL;
    }

    if (format == ARGOS_STORE_FORMAT_CSV) {
        fputs("timestamp,channel,value_raw,value_mv\n", f);
    } else {
        fputs("[\n", f);
    }
    fclose(f);

    STORE_LOG("Created log file: %s", s_current_log_file);
    xSemaphoreGive(s_file_mutex);
    return ESP_OK;
}

esp_err_t argos_store_close_log_file(void) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    argos_store_flush();

    if (xSemaphoreTake(s_file_mutex, portMAX_DELAY) == pdTRUE) {
        s_current_log_file[0] = '\0';
        xSemaphoreGive(s_file_mutex);
    }

    STORE_LOG("Log file closed");
    return ESP_OK;
}

esp_err_t argos_store_list_files(argos_store_file_info_t *files, size_t max_files, size_t *count) {
    if (!s_initialized || files == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *count = 0;
    DIR *dir = opendir(ARGOS_STORE_BASE_PATH);
    if (dir == NULL) {
        STORE_ERR("Failed to open directory: %s", ARGOS_STORE_BASE_PATH);
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *count < max_files) {
        if (entry->d_type == DT_REG) {
            char path[128];
            snprintf(path, sizeof(path), "%s/%s", ARGOS_STORE_BASE_PATH, entry->d_name);

            struct stat st;
            if (stat(path, &st) == 0) {
                strncpy(files[*count].filename, entry->d_name, sizeof(files[*count].filename) - 1);
                files[*count].size = st.st_size;
                files[*count].timestamp = st.st_mtime;
                files[*count].record_count = 0;
                (*count)++;
            }
        }
    }

    closedir(dir);
    STORE_LOG("Found %d log files", *count);
    return ESP_OK;
}

esp_err_t argos_store_delete_oldest(void) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    argos_store_file_info_t files[ARGOS_STORE_MAX_LOG_FILES];
    size_t count = 0;

    esp_err_t ret = argos_store_list_files(files, ARGOS_STORE_MAX_LOG_FILES, &count);
    if (ret != ESP_OK || count == 0) return ret;

    size_t oldest_idx = 0;
    for (size_t i = 1; i < count; i++) {
        if (files[i].timestamp < files[oldest_idx].timestamp) {
            oldest_idx = i;
        }
    }

    char path[128];
    snprintf(path, sizeof(path), "%s/%s", ARGOS_STORE_BASE_PATH, files[oldest_idx].filename);

    if (unlink(path) == 0) {
        STORE_LOG("Deleted oldest log: %s (%d bytes)", files[oldest_idx].filename, files[oldest_idx].size);
        return ESP_OK;
    }

    STORE_ERR("Failed to delete file: %s", path);
    return ESP_FAIL;
}

esp_err_t argos_store_export_file(const char *filename, uint8_t *buffer, size_t buffer_size, size_t *bytes_read) {
    if (!s_initialized || filename == NULL || buffer == NULL || bytes_read == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char path[128];
    snprintf(path, sizeof(path), "%s/%s", ARGOS_STORE_BASE_PATH, filename);

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        STORE_ERR("Failed to open file for export: %s", path);
        return ESP_FAIL;
    }

    *bytes_read = fread(buffer, 1, buffer_size, f);
    fclose(f);

    return ESP_OK;
}

esp_err_t argos_store_get_stats(argos_store_stats_t *stats) {
    if (!s_initialized || stats == NULL) return ESP_ERR_INVALID_ARG;
    return get_partition_stats(stats);
}

esp_err_t argos_store_check_rotation(void) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    argos_store_stats_t stats;
    esp_err_t ret = argos_store_get_stats(&stats);
    if (ret != ESP_OK) return ret;

    if (stats.usage_percent >= s_critical_threshold) {
        STORE_ERR("CRITICAL: Partition %d%% full (threshold %d%%)",
                  stats.usage_percent, s_critical_threshold);
        s_critical_state = true;
        if (s_event_cb) s_event_cb(1, s_event_cb_arg);
        return ESP_ERR_NO_MEM;
    }

    while (stats.usage_percent >= s_warning_threshold) {
        s_critical_state = false;
        STORE_WARN("Partition %d%% full, rotating...", stats.usage_percent);

        argos_store_close_log_file();
        ret = argos_store_delete_oldest();
        if (ret != ESP_OK) break;
        ret = argos_store_create_log_file(ARGOS_STORE_FORMAT_CSV);
        if (ret != ESP_OK) break;

        if (s_event_cb) s_event_cb(2, s_event_cb_arg);

        ret = argos_store_get_stats(&stats);
        if (ret != ESP_OK) break;
    }

    return ESP_OK;
}

esp_err_t argos_store_set_thresholds(uint8_t warning_pct, uint8_t critical_pct) {
    if (warning_pct >= critical_pct || critical_pct >= 100) {
        return ESP_ERR_INVALID_ARG;
    }
    s_warning_threshold = warning_pct;
    s_critical_threshold = critical_pct;
    STORE_LOG("Thresholds set: warning=%d%%, critical=%d%%", warning_pct, critical_pct);
    return ESP_OK;
}

void argos_store_print_diagnostics(void) {
    STORE_LOG("========== STORE DIAGNOSTICS ==========");
    STORE_LOG("Initialized: %s", s_initialized ? "YES" : "NO");
    STORE_LOG("Buffer: %d KB, usage: %d bytes", ARGOS_STORE_BUFFER_SIZE / 1024, s_circular_buf.count);
    STORE_LOG("Current file: %s", s_current_log_file[0] ? s_current_log_file : "None");
    STORE_LOG("Thresholds: warning=%d%%, critical=%d%%", s_warning_threshold, s_critical_threshold);
    STORE_LOG("Critical state: %s", s_critical_state ? "YES (READ-ONLY)" : "NO");

    argos_store_stats_t stats;
    if (argos_store_get_stats(&stats) == ESP_OK) {
        STORE_LOG("Partition: total=%d KB, used=%d KB, free=%d KB (%d%%)",
                  stats.total_bytes / 1024, stats.used_bytes / 1024,
                  stats.free_bytes / 1024, stats.usage_percent);
    }

    argos_store_file_info_t files[10];
    size_t count = 0;
    if (argos_store_list_files(files, 10, &count) == ESP_OK) {
        STORE_LOG("Log files (%d):", count);
        for (size_t i = 0; i < count && i < 5; i++) {
            STORE_LOG("  %s (%d bytes)", files[i].filename, files[i].size);
        }
        if (count > 5) STORE_LOG("  ... and %d more", count - 5);
    }
    STORE_LOG("======================================");
}

esp_err_t argos_store_self_test(void) {
    STORE_LOG("Starting store self-test...");

    if (!s_initialized) {
        STORE_ERR("Self-test failed: store not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    argos_store_stats_t stats;
    esp_err_t ret = argos_store_get_stats(&stats);
    if (ret != ESP_OK) {
        STORE_ERR("Self-test failed: cannot get stats");
        return ret;
    }
    STORE_LOG("Self-test: partition stats OK (%d%% used)", stats.usage_percent);

    argos_store_file_info_t files[5];
    size_t count = 0;
    ret = argos_store_list_files(files, 5, &count);
    if (ret != ESP_OK) {
        STORE_ERR("Self-test failed: cannot list files");
        return ret;
    }
    STORE_LOG("Self-test: file listing OK (%d files)", count);

    argos_measurement_t test_meas = {
        .timestamp = 1000,
        .channel = 0,
        .value = 1.5f
    };
    ret = argos_store_write_measurement(&test_meas);
    if (ret != ESP_OK) {
        STORE_ERR("Self-test failed: cannot write measurement");
        return ret;
    }
    STORE_LOG("Self-test: measurement write OK");

    ret = argos_store_flush();
    if (ret != ESP_OK) {
        STORE_ERR("Self-test failed: cannot flush");
        return ret;
    }
    STORE_LOG("Self-test: flush OK");

    ret = argos_store_check_rotation();
    if (ret != ESP_OK && ret != ESP_ERR_NO_MEM) {
        STORE_ERR("Self-test: rotation check returned %s", esp_err_to_name(ret));
    } else {
        STORE_LOG("Self-test: rotation check OK");
    }

    STORE_LOG("Store self-test PASSED");
    return ESP_OK;
}

esp_err_t argos_store_format(void) {
    STORE_LOG("WARN: Formatting partition '%s'...", ARGOS_STORE_PARTITION_LABEL);

    if (s_initialized) {
        argos_store_deinit();
    }

    esp_err_t ret = esp_littlefs_format(ARGOS_STORE_PARTITION_LABEL);
    if (ret != ESP_OK) {
        STORE_ERR("Format failed: %s", esp_err_to_name(ret));
        return ret;
    }

    STORE_LOG("Partition formatted successfully");
    return ESP_OK;
}

void argos_store_register_callback(argos_store_event_cb_t cb, void *arg) {
    s_event_cb = cb;
    s_event_cb_arg = arg;
    STORE_LOG("Event callback registered");
}
