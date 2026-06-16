#include "argos_hal.h"
#include "argos_core.h"
#include "argos_store.h"
#include "argos_net.h"
#include "argos_router.h"
#include "experiment_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "argos_main";

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Argos - Framework de Instrumentación v2.0");

    argos_config_t config = {0};
    argos_core_init(&config);

    /* ==================== FASE 1: Inicializar HAL ==================== */
    ESP_LOGI(TAG, "[1/4] Inicializando HAL...");
    ESP_ERROR_CHECK(argos_hal_adc_init(&config));
    ESP_ERROR_CHECK(argos_hal_dac_init());
    ESP_ERROR_CHECK(argos_hal_pwm_init());
    ESP_ERROR_CHECK(argos_hal_self_test());

    /* ==================== FASE 2: Inicializar almacenamiento ==================== */
    ESP_LOGI(TAG, "[2/4] Inicializando almacenamiento...");
    ESP_ERROR_CHECK(argos_store_init());

    /* ==================== FASE 3: Inicializar red ==================== */
    ESP_LOGI(TAG, "[3/4] Inicializando red...");
    ESP_ERROR_CHECK(argos_net_init());

    /* ==================== FASE 4: Inicializar enrutador ==================== */
    ESP_LOGI(TAG, "[4/4] Inicializando enrutador...");

    /* Cargar configuración por defecto */
    argos_experiment_config_t exp_cfg;
    exp_config_init_default(&exp_cfg);
    strcpy(exp_cfg.nombre, "Adquisición general");
    strcpy(exp_cfg.descripcion, "Captura de 4 canales ADC con almacenamiento y WebSocket");

    /* Configurar columnas personalizadas */
    exp_cfg.num_columnas = 6;
    exp_cfg.columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    exp_cfg.columnas[1] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
    exp_cfg.columnas[2] = (exp_column_config_t){EXP_COL_VALOR_RAW, "valor_crudo", true};
    exp_cfg.columnas[3] = (exp_column_config_t){EXP_COL_VALOR_MV, "voltaje_mV", true};
    exp_cfg.columnas[4] = (exp_column_config_t){EXP_COL_VALOR_ESCALADO, "valor_escalado", false};
    exp_cfg.columnas[5] = (exp_column_config_t){EXP_COL_DAC_OUT, "dac_salida_mV", false};

    ESP_ERROR_CHECK(argos_router_init(&exp_cfg));
    ESP_ERROR_CHECK(argos_router_start_acquisition_task());

    /* ==================== LISTO ==================== */
    ESP_LOGI(TAG, "=== Argos listo ===");
    ESP_LOGI(TAG, "Conéctese a WiFi: %s", ARGOS_NET_AP_SSID);
    ESP_LOGI(TAG, "Abra http://%s en su navegador", ARGOS_NET_AP_IP);

    /* Listar plantillas disponibles */
    char temps[EXP_CONFIG_TEMPLATE_MAX][EXP_CONFIG_TEMPLATE_NAME_MAX];
    size_t num_temps = 0;
    exp_config_list_templates(temps, EXP_CONFIG_TEMPLATE_MAX, &num_temps);
    ESP_LOGI(TAG, "Plantillas disponibles (%d):", num_temps);
    for (size_t i = 0; i < num_temps; i++) {
        ESP_LOGI(TAG, "  [%d] %s", i, temps[i]);
    }
    ESP_LOGI(TAG, "Use: exp_config_apply_template(&cfg, \"barrido_dac\") para cambiar");

    /* Iniciar experimento automáticamente */
    ESP_LOGI(TAG, "Iniciando experimento: %s", exp_cfg.nombre);
    ESP_ERROR_CHECK(argos_router_start_experiment());

    /* Bucle principal de monitoreo */
    while (1) {
        argos_router_stats_t stats;
        if (argos_router_get_stats(&stats) == ESP_OK) {
            ESP_LOGI(TAG, "Monitor: %lu mediciones, %lu err, %lu med/s",
                     stats.total_mediciones, stats.errores, stats.mediciones_segundo);
        }

        /* Mostrar diagnóstico cada 30 segundos */
        static uint32_t contador = 0;
        if (++contador % 30 == 0) {
            argos_hal_print_diagnostics();
            argos_store_print_diagnostics();
            argos_net_print_diagnostics();
            argos_router_print_diagnostics();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
