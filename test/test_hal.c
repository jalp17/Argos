#include "unity.h"
#include "argos_hal.h"
#include "argos_core.h"
#include "hw_config.h"
#include "esp_log.h"

static const char *TAG = "test_hal";

TEST_CASE("HAL ADC initialization", "[argos_hal][adc]") {
    argos_config_t config = {0};
    argos_core_init(&config);
    
    esp_err_t ret = argos_hal_adc_init(&config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    HAL_DEBUG_LOG("ADC init test passed");
}

TEST_CASE("HAL ADC read raw", "[argos_hal][adc]") {
    int raw = 0;
    esp_err_t ret = argos_hal_adc_read_raw(ADC_CHANNEL_0, &raw);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(raw >= 0 && raw <= 4095);
    
    HAL_DEBUG_LOG("ADC read raw test passed: %d", raw);
}

TEST_CASE("HAL ADC read voltage", "[argos_hal][adc]") {
    uint32_t voltage = 0;
    esp_err_t ret = argos_hal_adc_read_voltage(ADC_CHANNEL_0, &voltage);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(voltage <= 3300);
    
    HAL_DEBUG_LOG("ADC read voltage test passed: %lu mV", voltage);
}

TEST_CASE("HAL ADC multi-read", "[argos_hal][adc]") {
    adc_channel_t channels[] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5};
    uint32_t voltages[4] = {0};
    
    esp_err_t ret = argos_hal_adc_read_multi(channels, 4, voltages);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(voltages[i] <= 3300);
        HAL_DEBUG_LOG("Channel %d: %lu mV", channels[i], voltages[i]);
    }
}

TEST_CASE("HAL DAC initialization", "[argos_hal][dac]") {
    esp_err_t ret = argos_hal_dac_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    HAL_DEBUG_LOG("DAC init test passed");
}

TEST_CASE("HAL DAC write value", "[argos_hal][dac]") {
    esp_err_t ret = argos_hal_dac_write(DAC_CHAN_0, 128);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = argos_hal_dac_write(DAC_CHAN_0, 0);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    HAL_DEBUG_LOG("DAC write test passed");
}

TEST_CASE("HAL DAC write voltage", "[argos_hal][dac]") {
    esp_err_t ret = argos_hal_dac_write_voltage(DAC_CHAN_0, 1650);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = argos_hal_dac_write_voltage(DAC_CHAN_0, 0);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    HAL_DEBUG_LOG("DAC write voltage test passed");
}

TEST_CASE("HAL PWM initialization", "[argos_hal][pwm]") {
    esp_err_t ret = argos_hal_pwm_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    HAL_DEBUG_LOG("PWM init test passed");
}

TEST_CASE("HAL PWM config and duty", "[argos_hal][pwm]") {
    esp_err_t ret = argos_hal_pwm_config_channel(0, GPIO_NUM_18, 10000, 4096);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = argos_hal_pwm_set_duty(0, 2048);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = argos_hal_pwm_set_duty(0, 0);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    HAL_DEBUG_LOG("PWM config and duty test passed");
}

TEST_CASE("HAL self-test", "[argos_hal][diagnostics]") {
    esp_err_t ret = argos_hal_self_test();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    HAL_DEBUG_LOG("Self-test passed");
}

TEST_CASE("HAL diagnostics print", "[argos_hal][diagnostics]") {
    argos_hal_print_diagnostics();
    TEST_PASS();
    
    HAL_DEBUG_LOG("Diagnostics print test passed");
}
