#ifndef ARGOS_HAL_H
#define ARGOS_HAL_H

#include "driver/adc.h"
#include "soc/soc_caps.h"
#if SOC_DAC_SUPPORTED
#include "driver/dac.h"
#endif
#include "driver/ledc.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "argos_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hardware Abstraction Layer para Argos.
 * Proporciona abstracción para ADC, DAC y PWM.
 */

/* ==================== ADC ==================== */

/**
 * @brief Inicializa el subsistema ADC.
 * @param config Configuración global de Argos.
 * @return ESP_OK en éxito, código de error en fallo.
 */
esp_err_t argos_hal_adc_init(const argos_config_t *config);

/**
 * @brief Lee un canal ADC raw.
 * @param channel Canal ADC a leer.
 * @param raw_value Puntero para almacenar valor raw.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_adc_read_raw(adc_channel_t channel, int *raw_value);

/**
 * @brief Lee un canal ADC y convierte a voltaje (mV).
 * @param channel Canal ADC a leer.
 * @param voltage_mv Puntero para almacenar voltaje en mV.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_adc_read_voltage(adc_channel_t channel, uint32_t *voltage_mv);

/**
 * @brief Lee múltiples canales ADC.
 * @param channels Array de canales a leer.
 * @param count Número de canales.
 * @param voltages Array para almacenar voltajes en mV.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_adc_read_multi(const adc_channel_t *channels, size_t count, uint32_t *voltages);

/**
 * @brief Configura la atenuación del ADC.
 * @param channel Canal ADC.
 * @param atten Atenuación a configurar.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_adc_set_atten(adc_channel_t channel, adc_atten_t atten);

/* ==================== DAC ==================== */

#if SOC_DAC_SUPPORTED

esp_err_t argos_hal_dac_init(void);

esp_err_t argos_hal_dac_write(dac_channel_t channel, uint8_t value);

esp_err_t argos_hal_dac_write_voltage(dac_channel_t channel, uint32_t voltage_mv);

esp_err_t argos_hal_dac_enable(dac_channel_t channel, bool enable);

#endif /* SOC_DAC_SUPPORTED */

/* ==================== PWM (LEDC) ==================== */

/**
 * @brief Inicializa el subsistema PWM.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_pwm_init(void);

/**
 * @brief Configura un canal PWM.
 * @param channel Canal PWM (0-3).
 * @param gpio GPIO para el canal.
 * @param frequency_hz Frecuencia en Hz.
 * @param duty_cycle Ciclo de trabajo (0-8191 para 13-bit).
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_pwm_config_channel(uint8_t channel, gpio_num_t gpio, uint32_t frequency_hz, uint32_t duty_cycle);

/**
 * @brief Establece ciclo de trabajo PWM.
 * @param channel Canal PWM.
 * @param duty_cycle Ciclo de trabajo (0-8191).
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_pwm_set_duty(uint8_t channel, uint32_t duty_cycle);

/**
 * @brief Establece frecuencia PWM.
 * @param channel Canal PWM.
 * @param frequency_hz Frecuencia en Hz.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_pwm_set_frequency(uint8_t channel, uint32_t frequency_hz);

/**
 * @brief Inicia PWM en un canal.
 * @param channel Canal PWM.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_pwm_start(uint8_t channel);

/**
 * @brief Detiene PWM en un canal.
 * @param channel Canal PWM.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_pwm_stop(uint8_t channel);

/* ==================== DEBUG & DIAGNOSTICS ==================== */

/**
 * @brief Imprime información de diagnóstico del HAL.
 */
void argos_hal_print_diagnostics(void);

/**
 * @brief Realiza auto-test del hardware.
 * @return ESP_OK si todo funciona correctamente.
 */
esp_err_t argos_hal_self_test(void);

/**
 * @brief Obtiene calibración ADC para un canal.
 * @param channel Canal ADC.
 * @param cal_handle Puntero para manejador de calibración.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_hal_adc_get_calibration(adc_channel_t channel, adc_cali_handle_t *cal_handle);

#ifdef __cplusplus
}
#endif

#endif // ARGOS_HAL_H
