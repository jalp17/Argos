#include "argos_hal.h"
#include "argos_core.h"
#include "argos_store.h"
#include "argos_net.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "argos_main";

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Argos - Framework de Instrumentación");

    argos_config_t config = {0};
    argos_core_init(&config);

    /* Inicializar HAL (ADC, DAC, PWM) */
    ESP_LOGI(TAG, "Inicializando HAL...");
    ESP_ERROR_CHECK(argos_hal_adc_init(&config));
    ESP_ERROR_CHECK(argos_hal_dac_init());
    ESP_ERROR_CHECK(argos_hal_pwm_init());
    ESP_ERROR_CHECK(argos_hal_self_test());

    /* Inicializar almacenamiento (LittleFS + buffer circular) */
    ESP_LOGI(TAG, "Inicializando almacenamiento...");
    ESP_ERROR_CHECK(argos_store_init());

    /* Inicializar red (SoftAP + Servidor Web + WebSockets) */
    ESP_LOGI(TAG, "Inicializando red...");
    ESP_ERROR_CHECK(argos_net_init());

    ESP_LOGI(TAG, "=== Argos listo ===");
    ESP_LOGI(TAG, "Conéctese a WiFi: %s", ARGOS_NET_AP_SSID);
    ESP_LOGI(TAG, "Abra http://%s en su navegador", ARGOS_NET_AP_IP);

    /* Bucle principal: lectura ADC, almacenamiento y envío WebSocket */
    adc_channel_t canales[] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5};
    uint32_t voltajes[4] = {0};
    uint32_t contador = 0;

    while (1) {
        esp_err_t ret = argos_hal_adc_read_multi(canales, 4, voltajes);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "ADC: CH0=%lu mV, CH1=%lu mV, CH2=%lu mV, CH3=%lu mV",
                     voltajes[0], voltajes[1], voltajes[2], voltajes[3]);

            /* Almacenar cada canal */
            for (int i = 0; i < 4; i++) {
                argos_measurement_t medicion = {
                    .timestamp = contador + i,
                    .channel = (uint8_t)i,
                    .value = voltajes[i] / 1000.0f
                };
                argos_store_write_measurement(&medicion);

                /* Enviar por WebSocket a clientes conectados */
                argos_net_ws_send_measurement(&medicion);
            }
            contador += 4;

            /* Verificar rotación cada 40 mediciones */
            if (contador % 40 == 0) {
                argos_store_check_rotation();
                argos_hal_print_diagnostics();
                argos_store_print_diagnostics();
                argos_net_print_diagnostics();
            }
        }

        /* Generar señal de prueba en DAC */
        static uint8_t dac_val = 0;
        argos_hal_dac_write_voltage(DAC_CHAN_0, (dac_val * 3300) / 255);
        dac_val = (dac_val + 10) % 256;

        /* Actualizar PWM */
        argos_hal_pwm_set_duty(0, (dac_val * 8191) / 255);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
