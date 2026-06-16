#include "argos_hal.h"
#include "argos_core.h"
#include "argos_store.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "Starting Argos - Instrumentation Framework");

    argos_config_t config = {0};
    argos_core_init(&config);

    ESP_LOGI(TAG, "Initializing HAL components...");
    ESP_ERROR_CHECK(argos_hal_adc_init(&config));
    ESP_ERROR_CHECK(argos_hal_dac_init());
    ESP_ERROR_CHECK(argos_hal_pwm_init());

    ESP_LOGI(TAG, "Running HAL self-test...");
    ESP_ERROR_CHECK(argos_hal_self_test());

    ESP_LOGI(TAG, "Initializing storage...");
    ESP_ERROR_CHECK(argos_store_init());
    ESP_ERROR_CHECK(argos_store_self_test());

    ESP_LOGI(TAG, "Starting continuous monitoring with data logging...");

    adc_channel_t channels[] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5};
    uint32_t voltages[4] = {0};
    uint32_t sample_count = 0;

    while (1) {
        esp_err_t ret = argos_hal_adc_read_multi(channels, 4, voltages);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "ADC: Ch0=%lu mV, Ch3=%lu mV, Ch4=%lu mV, Ch5=%lu mV",
                     voltages[0], voltages[1], voltages[2], voltages[3]);

            for (int i = 0; i < 4; i++) {
                argos_measurement_t meas = {
                    .timestamp = sample_count + i,
                    .channel = (uint8_t)i,
                    .value = voltages[i] / 1000.0f
                };
                argos_store_write_measurement(&meas);
            }
            sample_count += 4;

            if (sample_count % 40 == 0) {
                ESP_ERROR_CHECK(argos_store_check_rotation());
                argos_store_print_diagnostics();
            }
        }

        static uint8_t dac_val = 0;
        argos_hal_dac_write_voltage(DAC_CHAN_0, (dac_val * 3300) / 255);
        dac_val = (dac_val + 10) % 256;

        argos_hal_pwm_set_duty(0, (dac_val * 8191) / 255);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
