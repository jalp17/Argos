#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuración de pines y parámetros de hardware para Argos.
 * 
 * Centraliza toda la configuración de hardware para facilitar cambios
 * entre diferentes prácticas de laboratorio.
 */

/* ==================== ADC CONFIGURATION ==================== */
#define ARGOS_ADC_UNIT              ADC_UNIT_1
#define ARGOS_ADC_ATTEN             ADC_ATTEN_DB_11   // 0-3.3V range
#define ARGOS_ADC_WIDTH             ADC_WIDTH_BIT_12  // 12-bit resolution
#define ARGOS_ADC_CHANNELS          4                 // Number of ADC channels
#define ARGOS_ADC_DEFAULT_CHANNELS  {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5} // GPIO 36, 39, 32, 33
#define ARGOS_ADC_SAMPLE_RATE_HZ    10000             // Default sample rate
#define ARGOS_ADC_CAL_SCHEME        ADC_CAL_SCHEME_VER_CURVE_FITTING

/* ==================== DAC CONFIGURATION ==================== */
#define ARGOS_DAC_CHANNELS          2                 // Number of DAC channels
#define ARGOS_DAC_DEFAULT_CHANNELS  {DAC_CHAN_0, DAC_CHAN_1} // GPIO 25, 26
#define ARGOS_DAC_RESOLUTION        8                 // 8-bit resolution (0-255)
#define ARGOS_DAC_MAX_VOLTAGE_MV    3300              // Max output voltage in mV

/* ==================== PWM (LEDC) CONFIGURATION ==================== */
#define ARGOS_PWM_CHANNELS          4                 // Number of PWM channels
#define ARGOS_PWM_DEFAULT_GPIOS     {18, 19, 21, 22}  // Default GPIO pins
#define ARGOS_PWM_FREQUENCY_HZ      10000             // 10 kHz default
#define ARGOS_PWM_RESOLUTION_BITS   13                // 13-bit resolution (8192 levels)
#define ARGOS_PWM_TIMER             LEDC_TIMER_0
#define ARGOS_PWM_SPEED_MODE        LEDC_HIGH_SPEED_MODE

/* ==================== DEBUG CONFIGURATION ==================== */
#define ARGOS_HAL_DEBUG_ENABLE      1                 // Enable debug logging
#define ARGOS_HAL_DEBUG_TAG         "argos_hal"
#define ARGOS_HAL_VERBOSE_DEBUG     0                 // Verbose debug (register dumps, etc.)

#ifdef __cplusplus
}
#endif

#endif // HW_CONFIG_H
