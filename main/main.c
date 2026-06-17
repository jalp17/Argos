#include "argos_hal.h"
#include "argos_core.h"
#include "argos_store.h"
#include "argos_net.h"
#include "argos_router.h"
#include "experiment_config.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "argos_main";

#define ARGOS_WDT_TIMEOUT_SEC  10
#define ARGOS_MAIN_STACK_SIZE  4096
#define ARGOS_MONITOR_INTERVAL 30
#define ARGOS_HEAP_WARN_LEVEL  20480

static void imprimir_metricas_memoria(void) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    uint32_t libre = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    uint32_t minimo = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    uint32_t total = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    uint32_t usado = total - libre;

    ESP_LOGI(TAG, "Memoria: total=%lu KB, libre=%lu KB, min=%lu KB, usado=%lu%%",
             total / 1024, libre / 1024, minimo / 1024,
             total > 0 ? (usado * 100) / total : 0);

    if (libre < ARGOS_HEAP_WARN_LEVEL) {
        ESP_LOGW(TAG, "Memoria baja: solo %lu bytes libres", libre);
    }
}

static void inicializar_watchdogs(void) {
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = ARGOS_WDT_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    esp_err_t ret = esp_task_wdt_init(&wdt_config);
    if (ret == ESP_OK) {
        ret = esp_task_wdt_add(NULL);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "TWDT configurado: %d ms (%d s)", wdt_config.timeout_ms, ARGOS_WDT_TIMEOUT_SEC);
        } else {
            ESP_LOGW(TAG, "No se pudo agregar tarea al TWDT: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGW(TAG, "No se pudo inicializar TWDT: %s", esp_err_to_name(ret));
    }
}

static void finalizar_ordenadamente(void) {
    ESP_LOGW(TAG, "Iniciando apagado ordenado...");

    if (argos_router_get_state() == EXP_STATE_CORRIENDO) {
        argos_router_stop_experiment();
    }

    argos_router_deinit();
    argos_net_deinit();
    argos_store_deinit();
    argos_hal_print_diagnostics();

    esp_task_wdt_deinit();
    ESP_LOGI(TAG, "Apagado completado");
}

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Argos - Framework de Instrumentación v2.1");
    ESP_LOGI(TAG, "ESP-IDF v5.3, FreeRTOS, SPIFFS, WebSocket");

    inicializar_watchdogs();
    imprimir_metricas_memoria();

    argos_config_t config = {0};
    argos_core_init(&config);

    bool hal_ok = false, store_ok = false, net_ok = false, router_ok = false;

    /* FASE 1: Inicializar HAL */
    ESP_LOGI(TAG, "[1/4] Inicializando HAL...");
    esp_err_t ret;

    ret = argos_hal_adc_init(&config);
    if (ret == ESP_OK) ret = argos_hal_dac_init();
    if (ret == ESP_OK) ret = argos_hal_pwm_init();
    if (ret == ESP_OK) ret = argos_hal_self_test();

    if (ret == ESP_OK) {
        hal_ok = true;
        ESP_LOGI(TAG, "HAL inicializado correctamente");
    } else {
        ESP_LOGE(TAG, "Error en HAL: %s, continuando sin ADC/DAC/PWM", esp_err_to_name(ret));
    }

    /* FASE 2: Inicializar almacenamiento */
    ESP_LOGI(TAG, "[2/4] Inicializando almacenamiento...");
    ret = argos_store_init();
    if (ret == ESP_OK) {
        ret = argos_store_self_test();
    }
    if (ret == ESP_OK) {
        store_ok = true;
        ESP_LOGI(TAG, "Almacenamiento listo");
    } else {
        ESP_LOGE(TAG, "Error en almacenamiento: %s, continuando sin SPIFFS", esp_err_to_name(ret));
    }

    /* FASE 3: Inicializar red */
    ESP_LOGI(TAG, "[3/4] Inicializando red...");
    ret = argos_net_init();
    if (ret == ESP_OK) {
        net_ok = true;
        ESP_LOGI(TAG, "Red lista: SSID=%s IP=%s", ARGOS_NET_AP_SSID, ARGOS_NET_AP_IP);
    } else {
        ESP_LOGE(TAG, "Error en red: %s, continuando sin servidor web", esp_err_to_name(ret));
    }

    /* FASE 4: Inicializar enrutador */
    ESP_LOGI(TAG, "[4/4] Inicializando enrutador...");

    argos_experiment_config_t exp_cfg;
    exp_config_init_default(&exp_cfg);
    strcpy(exp_cfg.nombre, "Adquisición general");
    strcpy(exp_cfg.descripcion, "Captura de 4 canales ADC con almacenamiento y WebSocket");

    exp_cfg.num_columnas = 6;
    exp_cfg.columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    exp_cfg.columnas[1] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
    exp_cfg.columnas[2] = (exp_column_config_t){EXP_COL_VALOR_RAW, "valor_crudo", true};
    exp_cfg.columnas[3] = (exp_column_config_t){EXP_COL_VALOR_MV, "voltaje_mV", true};
    exp_cfg.columnas[4] = (exp_column_config_t){EXP_COL_VALOR_ESCALADO, "valor_escalado", false};
    exp_cfg.columnas[5] = (exp_column_config_t){EXP_COL_DAC_OUT, "dac_salida_mV", false};

    ret = argos_router_init(&exp_cfg);
    if (ret == ESP_OK) {
        ret = argos_router_start_acquisition_task();
    }
    if (ret == ESP_OK) {
        router_ok = true;
        ESP_LOGI(TAG, "Enrutador listo");
    } else {
        ESP_LOGE(TAG, "Error en enrutador: %s, continuando sin adquisición", esp_err_to_name(ret));
    }

    /* Métricas de memoria tras inicialización */
    ESP_LOGI(TAG, "=== Argos listo ===");
    ESP_LOGI(TAG, "Estado: HAL=%s, Store=%s, Net=%s, Router=%s",
             hal_ok ? "OK" : "FALLO", store_ok ? "OK" : "FALLO",
             net_ok ? "OK" : "FALLO", router_ok ? "OK" : "FALLO");

    if (net_ok) {
        ESP_LOGI(TAG, "Conéctese a WiFi: %s", ARGOS_NET_AP_SSID);
        ESP_LOGI(TAG, "Abra http://%s en su navegador", ARGOS_NET_AP_IP);
    }

    imprimir_metricas_memoria();

    /* Iniciar experimento */
    if (router_ok) {
        ESP_LOGI(TAG, "Iniciando experimento: %s", exp_cfg.nombre);
        argos_router_start_experiment();

        char temps[EXP_CONFIG_TEMPLATE_MAX][EXP_CONFIG_TEMPLATE_NAME_MAX];
        size_t num_temps = 0;
        exp_config_list_templates(temps, EXP_CONFIG_TEMPLATE_MAX, &num_temps);
        ESP_LOGI(TAG, "Plantillas disponibles (%d):", num_temps);
        for (size_t i = 0; i < num_temps; i++) {
            ESP_LOGI(TAG, "  [%d] %s", i, temps[i]);
        }
    }

    uint32_t contador = 0;

    while (1) {
        esp_task_wdt_reset();

        if (router_ok) {
            argos_router_stats_t stats;
            if (argos_router_get_stats(&stats) == ESP_OK) {
                ESP_LOGI(TAG, "Monitor: %lu mediciones, %lu err, %lu med/s",
                         stats.total_mediciones, stats.errores, stats.mediciones_segundo);
            }
        }

        contador++;
        if (contador % ARGOS_MONITOR_INTERVAL == 0) {
            if (hal_ok) argos_hal_print_diagnostics();
            if (store_ok) argos_store_print_diagnostics();
            if (net_ok) argos_net_print_diagnostics();
            if (router_ok) argos_router_print_diagnostics();
            imprimir_metricas_memoria();

            if (store_ok) {
                argos_store_check_rotation();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    finalizar_ordenadamente();
}
