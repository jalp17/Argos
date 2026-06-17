#include "argos_hal.h"
#include "hw_config.h"
#include "esp_log.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = ARGOS_HAL_DEBUG_TAG;

static adc_channel_t s_adc_channels[ARGOS_ADC_CHANNELS] = ARGOS_ADC_DEFAULT_CHANNELS;
static dac_channel_t s_dac_channels[ARGOS_DAC_CHANNELS] = ARGOS_DAC_DEFAULT_CHANNELS;
static gpio_num_t s_pwm_gpios[ARGOS_PWM_CHANNELS] = ARGOS_PWM_DEFAULT_GPIOS;

static adc_cali_handle_t s_adc_chars[ARGOS_ADC_CHANNELS] = {NULL};
static bool s_adc_initialized = false;
static bool s_dac_initialized = false;
static bool s_pwm_initialized = false;

#if ARGOS_HAL_DEBUG_ENABLE
#define HAL_DEBUG_LOG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define HAL_DEBUG_ERR(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define HAL_DEBUG_WARN(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
#define HAL_DEBUG_LOG(fmt, ...)
#define HAL_DEBUG_ERR(fmt, ...)
#define HAL_DEBUG_WARN(fmt, ...)
#endif

#if ARGOS_HAL_VERBOSE_DEBUG
#define HAL_VERBOSE_LOG(fmt, ...) ESP_LOGI(TAG, "[VERBOSE] " fmt, ##__VA_ARGS__)
#else
#define HAL_VERBOSE_LOG(fmt, ...)
#endif

/* ==================== ADC IMPLEMENTATION ==================== */

esp_err_t argos_hal_adc_init(const argos_config_t *config) {
    if (s_adc_initialized) {
        HAL_DEBUG_WARN("ADC already initialized");
        return ESP_OK;
    }

    HAL_DEBUG_LOG("Initializing ADC...");

    esp_err_t ret = adc1_config_width(ARGOS_ADC_WIDTH);
    if (ret != ESP_OK) {
        HAL_DEBUG_ERR("Failed to configure ADC width: %s", esp_err_to_name(ret));
        return ret;
    }

    for (int i = 0; i < ARGOS_ADC_CHANNELS; i++) {
        ret = adc1_config_channel_atten(s_adc_channels[i], ARGOS_ADC_ATTEN);
        if (ret != ESP_OK) {
            HAL_DEBUG_ERR("Failed to configure channel %d atten: %s", s_adc_channels[i], esp_err_to_name(ret));
            return ret;
        }

        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ARGOS_ADC_UNIT,
            .atten = ARGOS_ADC_ATTEN,
            .bitwidth = ARGOS_ADC_WIDTH,
            .default_vref = 1100,
        };

        ret = adc_cali_create_scheme_line_fitting(&cali_config, &s_adc_chars[i]);
        if (ret != ESP_OK || s_adc_chars[i] == NULL) {
            HAL_DEBUG_ERR("Failed to create ADC calibration for channel %d", s_adc_channels[i]);
            return ESP_ERR_NO_MEM;
        }

        HAL_DEBUG_LOG("ADC Channel %d calibration created (line fitting)", s_adc_channels[i]);
    }

    s_adc_initialized = true;
    HAL_DEBUG_LOG("ADC initialized successfully (%d channels)", ARGOS_ADC_CHANNELS);
    return ESP_OK;
}

esp_err_t argos_hal_adc_read_raw(adc_channel_t channel, int *raw_value) {
    if (!s_adc_initialized) {
        HAL_DEBUG_ERR("ADC not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (raw_value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *raw_value = adc1_get_raw(channel);
    HAL_VERBOSE_LOG("ADC Channel %d raw: %d", channel, *raw_value);
    return ESP_OK;
}

esp_err_t argos_hal_adc_read_voltage(adc_channel_t channel, uint32_t *voltage_mv) {
    if (!s_adc_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (voltage_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw = 0;
    esp_err_t ret = argos_hal_adc_read_raw(channel, &raw);
    if (ret != ESP_OK) {
        return ret;
    }

    int channel_idx = -1;
    for (int i = 0; i < ARGOS_ADC_CHANNELS; i++) {
        if (s_adc_channels[i] == channel) {
            channel_idx = i;
            break;
        }
    }

    adc_cali_handle_t handle = (channel_idx >= 0 && s_adc_chars[channel_idx] != NULL)
                                ? s_adc_chars[channel_idx] : s_adc_chars[0];
    int voltage = 0;
    esp_err_t ret_cali = adc_cali_raw_to_voltage(handle, raw, &voltage);
    if (ret_cali != ESP_OK) {
        HAL_DEBUG_ERR("ADC calibration conversion failed: %s", esp_err_to_name(ret_cali));
        return ESP_ERR_INVALID_STATE;
    }
    *voltage_mv = (uint32_t)voltage;

    HAL_VERBOSE_LOG("ADC Channel %d voltage: %lu mV", channel, *voltage_mv);
    return ESP_OK;
}

esp_err_t argos_hal_adc_read_multi(const adc_channel_t *channels, size_t count, uint32_t *voltages) {
    if (!s_adc_initialized || channels == NULL || voltages == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < count; i++) {
        esp_err_t ret = argos_hal_adc_read_voltage(channels[i], &voltages[i]);
        if (ret != ESP_OK) {
            HAL_DEBUG_ERR("Failed to read channel %d", channels[i]);
            return ret;
        }
    }
    return ESP_OK;
}

esp_err_t argos_hal_adc_set_atten(adc_channel_t channel, adc_atten_t atten) {
    if (!s_adc_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = adc1_config_channel_atten(channel, atten);
    if (ret == ESP_OK) {
        HAL_DEBUG_LOG("ADC Channel %d attenuation set to %d dB", channel, atten);
    }
    return ret;
}

esp_err_t argos_hal_adc_get_calibration(adc_channel_t channel, adc_cali_handle_t *cal_handle) {
    if (!s_adc_initialized || cal_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = 0; i < ARGOS_ADC_CHANNELS; i++) {
        if (s_adc_channels[i] == channel) {
            *cal_handle = s_adc_chars[i];
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

/* ==================== DAC IMPLEMENTATION ==================== */

esp_err_t argos_hal_dac_init(void) {
    if (s_dac_initialized) {
        HAL_DEBUG_WARN("DAC already initialized");
        return ESP_OK;
    }

    HAL_DEBUG_LOG("Initializing DAC...");

    for (int i = 0; i < ARGOS_DAC_CHANNELS; i++) {
        esp_err_t ret = dac_output_enable(s_dac_channels[i]);
        if (ret != ESP_OK) {
            HAL_DEBUG_ERR("Failed to enable DAC channel %d: %s", s_dac_channels[i], esp_err_to_name(ret));
            return ret;
        }
        dac_output_voltage(s_dac_channels[i], 0);
    }

    s_dac_initialized = true;
    HAL_DEBUG_LOG("DAC initialized successfully (%d channels)", ARGOS_DAC_CHANNELS);
    return ESP_OK;
}

esp_err_t argos_hal_dac_write(dac_channel_t channel, uint8_t value) {
    if (!s_dac_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = dac_output_voltage(channel, value);
    HAL_VERBOSE_LOG("DAC Channel %d write: %d", channel, value);
    return ret;
}

esp_err_t argos_hal_dac_write_voltage(dac_channel_t channel, uint32_t voltage_mv) {
    if (!s_dac_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (voltage_mv > ARGOS_DAC_MAX_VOLTAGE_MV) {
        HAL_DEBUG_WARN("DAC voltage %lu mV exceeds max %d mV, clamping", voltage_mv, ARGOS_DAC_MAX_VOLTAGE_MV);
        voltage_mv = ARGOS_DAC_MAX_VOLTAGE_MV;
    }

    uint8_t value = (uint8_t)((voltage_mv * 255) / ARGOS_DAC_MAX_VOLTAGE_MV);
    return argos_hal_dac_write(channel, value);
}

esp_err_t argos_hal_dac_enable(dac_channel_t channel, bool enable) {
    if (!s_dac_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = enable ? dac_output_enable(channel) : dac_output_disable(channel);
    HAL_DEBUG_LOG("DAC Channel %d %s", channel, enable ? "enabled" : "disabled");
    return ret;
}

/* ==================== PWM IMPLEMENTATION ==================== */

esp_err_t argos_hal_pwm_init(void) {
    if (s_pwm_initialized) {
        HAL_DEBUG_WARN("PWM already initialized");
        return ESP_OK;
    }

    HAL_DEBUG_LOG("Initializing PWM...");

    ledc_timer_config_t timer_conf = {
        .speed_mode = ARGOS_PWM_SPEED_MODE,
        .duty_resolution = ARGOS_PWM_RESOLUTION_BITS,
        .timer_num = ARGOS_PWM_TIMER,
        .freq_hz = ARGOS_PWM_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        HAL_DEBUG_ERR("Failed to configure LEDC timer: %s", esp_err_to_name(ret));
        return ret;
    }

    s_pwm_initialized = true;
    HAL_DEBUG_LOG("PWM timer initialized (freq: %d Hz, resolution: %d bits)", 
                  ARGOS_PWM_FREQUENCY_HZ, ARGOS_PWM_RESOLUTION_BITS);
    return ESP_OK;
}

esp_err_t argos_hal_pwm_config_channel(uint8_t channel, gpio_num_t gpio, uint32_t frequency_hz, uint32_t duty_cycle) {
    if (!s_pwm_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (channel >= ARGOS_PWM_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }

    ledc_channel_config_t channel_conf = {
        .gpio_num = gpio,
        .speed_mode = ARGOS_PWM_SPEED_MODE,
        .channel = channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = ARGOS_PWM_TIMER,
        .duty = duty_cycle,
        .hpoint = 0
    };

    esp_err_t ret = ledc_channel_config(&channel_conf);
    if (ret == ESP_OK) {
        s_pwm_gpios[channel] = gpio;
        HAL_DEBUG_LOG("PWM Channel %d configured (GPIO %d, freq: %lu Hz, duty: %lu)", 
                      channel, gpio, frequency_hz, duty_cycle);
    }
    return ret;
}

esp_err_t argos_hal_pwm_set_duty(uint8_t channel, uint32_t duty_cycle) {
    if (!s_pwm_initialized || channel >= ARGOS_PWM_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t max_duty = (1 << ARGOS_PWM_RESOLUTION_BITS) - 1;
    if (duty_cycle > max_duty) {
        duty_cycle = max_duty;
    }

    esp_err_t ret = ledc_set_duty(ARGOS_PWM_SPEED_MODE, channel, duty_cycle);
    if (ret == ESP_OK) {
        ret = ledc_update_duty(ARGOS_PWM_SPEED_MODE, channel);
    }
    HAL_VERBOSE_LOG("PWM Channel %d duty set to %lu", channel, duty_cycle);
    return ret;
}

esp_err_t argos_hal_pwm_set_frequency(uint8_t channel, uint32_t frequency_hz) {
    if (!s_pwm_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ledc_set_freq(ARGOS_PWM_SPEED_MODE, ARGOS_PWM_TIMER, frequency_hz);
    HAL_DEBUG_LOG("PWM frequency set to %lu Hz", frequency_hz);
    return ret;
}

esp_err_t argos_hal_pwm_start(uint8_t channel) {
    if (!s_pwm_initialized || channel >= ARGOS_PWM_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ledc_update_duty(ARGOS_PWM_SPEED_MODE, channel);
    HAL_DEBUG_LOG("PWM Channel %d started", channel);
    return ret;
}

esp_err_t argos_hal_pwm_stop(uint8_t channel) {
    if (!s_pwm_initialized || channel >= ARGOS_PWM_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ledc_stop(ARGOS_PWM_SPEED_MODE, channel, 0);
    HAL_DEBUG_LOG("PWM Channel %d stopped", channel);
    return ret;
}

/* ==================== DEBUG & DIAGNOSTICS ==================== */

void argos_hal_print_diagnostics(void) {
    HAL_DEBUG_LOG("========== HAL DIAGNOSTICS ==========");
    HAL_DEBUG_LOG("ADC Initialized: %s", s_adc_initialized ? "YES" : "NO");
    HAL_DEBUG_LOG("DAC Initialized: %s", s_dac_initialized ? "YES" : "NO");
    HAL_DEBUG_LOG("PWM Initialized: %s", s_pwm_initialized ? "YES" : "NO");

    if (s_adc_initialized) {
        HAL_DEBUG_LOG("--- ADC Channels ---");
        for (int i = 0; i < ARGOS_ADC_CHANNELS; i++) {
            int raw = 0;
            uint32_t voltage = 0;
            argos_hal_adc_read_raw(s_adc_channels[i], &raw);
            argos_hal_adc_read_voltage(s_adc_channels[i], &voltage);
            HAL_DEBUG_LOG("  Channel %d (ADC%d): Raw=%d, Voltage=%lu mV", 
                          i, s_adc_channels[i], raw, voltage);
        }
    }

    if (s_pwm_initialized) {
        HAL_DEBUG_LOG("--- PWM Channels ---");
        for (int i = 0; i < ARGOS_PWM_CHANNELS; i++) {
            uint32_t duty = ledc_get_duty(ARGOS_PWM_SPEED_MODE, i);
            HAL_DEBUG_LOG("  Channel %d (GPIO %d): Duty=%lu", i, s_pwm_gpios[i], duty);
        }
    }
    HAL_DEBUG_LOG("=====================================");
}

esp_err_t argos_hal_self_test(void) {
    HAL_DEBUG_LOG("Starting HAL self-test...");
    esp_err_t ret = ESP_OK;

    if (!s_adc_initialized) {
        HAL_DEBUG_ERR("Self-test failed: ADC not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    for (int i = 0; i < ARGOS_ADC_CHANNELS; i++) {
        int raw = 0;
        uint32_t voltage = 0;
        ret = argos_hal_adc_read_raw(s_adc_channels[i], &raw);
        if (ret != ESP_OK) {
            HAL_DEBUG_ERR("Self-test failed: ADC channel %d read error", s_adc_channels[i]);
            return ret;
        }
        ret = argos_hal_adc_read_voltage(s_adc_channels[i], &voltage);
        if (ret != ESP_OK) {
            HAL_DEBUG_ERR("Self-test failed: ADC channel %d voltage conversion error", s_adc_channels[i]);
            return ret;
        }
        HAL_DEBUG_LOG("Self-test ADC channel %d: OK (raw=%d, voltage=%lu mV)", s_adc_channels[i], raw, voltage);
    }

    if (s_dac_initialized) {
        for (int i = 0; i < ARGOS_DAC_CHANNELS; i++) {
            ret = argos_hal_dac_write(s_dac_channels[i], 128);
            if (ret != ESP_OK) {
                HAL_DEBUG_ERR("Self-test failed: DAC channel %d write error", s_dac_channels[i]);
                return ret;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            ret = argos_hal_dac_write(s_dac_channels[i], 0);
            if (ret != ESP_OK) {
                HAL_DEBUG_ERR("Self-test failed: DAC channel %d clear error", s_dac_channels[i]);
                return ret;
            }
            HAL_DEBUG_LOG("Self-test DAC channel %d: OK", s_dac_channels[i]);
        }
    }

    if (s_pwm_initialized) {
        for (int i = 0; i < ARGOS_PWM_CHANNELS; i++) {
            ret = argos_hal_pwm_set_duty(i, (1 << (ARGOS_PWM_RESOLUTION_BITS - 1)));
            if (ret != ESP_OK) {
                HAL_DEBUG_ERR("Self-test failed: PWM channel %d duty error", i);
                return ret;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            ret = argos_hal_pwm_set_duty(i, 0);
            if (ret != ESP_OK) {
                HAL_DEBUG_ERR("Self-test failed: PWM channel %d clear error", i);
                return ret;
            }
            HAL_DEBUG_LOG("Self-test PWM channel %d: OK", i);
        }
    }

    HAL_DEBUG_LOG("HAL self-test PASSED");
    return ESP_OK;
}
