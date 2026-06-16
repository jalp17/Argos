#include "unity.h"
#include "argos_store.h"
#include "store_config.h"
#include "esp_log.h"

static const char *TAG = "test_store";

TEST_CASE("Store initialization", "[argos_store][init]") {
    esp_err_t ret = argos_store_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(argos_store_is_ready());
    STORE_LOG("Init test passed");
}

TEST_CASE("Store write measurement", "[argos_store][write]") {
    argos_measurement_t meas = {
        .timestamp = 12345,
        .channel = 0,
        .value = 1.65f
    };
    esp_err_t ret = argos_store_write_measurement(&meas);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    STORE_LOG("Write measurement test passed");
}

TEST_CASE("Store flush", "[argos_store][flush]") {
    esp_err_t ret = argos_store_flush();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    STORE_LOG("Flush test passed");
}

TEST_CASE("Store get stats", "[argos_store][stats]") {
    argos_store_stats_t stats;
    esp_err_t ret = argos_store_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(stats.total_bytes > 0);
    TEST_ASSERT_TRUE(stats.free_bytes > 0);
    TEST_ASSERT_TRUE(stats.usage_percent <= 100);
    STORE_LOG("Stats test passed: %d%% used", stats.usage_percent);
}

TEST_CASE("Store list files", "[argos_store][files]") {
    argos_store_file_info_t files[10];
    size_t count = 0;
    esp_err_t ret = argos_store_list_files(files, 10, &count);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    STORE_LOG("List files test passed: %d files", count);
}

TEST_CASE("Store check rotation", "[argos_store][rotation]") {
    esp_err_t ret = argos_store_check_rotation();
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_ERR_NO_MEM);
    STORE_LOG("Rotation check test passed");
}

TEST_CASE("Store set thresholds", "[argos_store][config]") {
    esp_err_t ret = argos_store_set_thresholds(80, 90);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    ret = argos_store_set_thresholds(85, 95);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    STORE_LOG("Thresholds test passed");
}

TEST_CASE("Store write measurements batch", "[argos_store][batch]") {
    argos_measurement_t measurements[10];
    for (int i = 0; i < 10; i++) {
        measurements[i].timestamp = 1000 + i;
        measurements[i].channel = i % 4;
        measurements[i].value = (float)(i * 100) / 1000.0f;
    }
    size_t written = argos_store_write_measurements(measurements, 10);
    TEST_ASSERT_EQUAL(10, written);
    argos_store_flush();
    STORE_LOG("Batch write test passed: %d measurements", written);
}

TEST_CASE("Store diagnostics", "[argos_store][diagnostics]") {
    argos_store_print_diagnostics();
    TEST_PASS();
    STORE_LOG("Diagnostics test passed");
}

TEST_CASE("Store self-test", "[argos_store][selftest]") {
    esp_err_t ret = argos_store_self_test();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    STORE_LOG("Self-test passed");
}

TEST_CASE("Store deinit", "[argos_store][deinit]") {
    esp_err_t ret = argos_store_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_FALSE(argos_store_is_ready());
    STORE_LOG("Deinit test passed");
}
