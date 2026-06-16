#include "unity.h"
#include "argos_net.h"
#include "net_config.h"
#include "esp_log.h"

static const char *TAG = "test_net";

TEST_CASE("Red: inicialización AP", "[argos_net][ap]") {
    esp_err_t ret = argos_net_ap_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    char ssid[32], ip[16];
    argos_net_get_ap_ssid(ssid);
    argos_net_get_ap_ip(ip);
    TEST_ASSERT_EQUAL_STRING(ARGOS_NET_AP_SSID, ssid);
    TEST_ASSERT_EQUAL_STRING(ARGOS_NET_AP_IP, ip);

    NET_LOG("AP init test OK: SSID=%s IP=%s", ssid, ip);
}

TEST_CASE("Red: inicialización servidor", "[argos_net][server]") {
    esp_err_t ret = argos_net_server_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(argos_net_is_ready());
    NET_LOG("Server init test OK");
}

TEST_CASE("Red: auto-test", "[argos_net][selftest]") {
    esp_err_t ret = argos_net_self_test();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    NET_LOG("Self-test OK");
}

TEST_CASE("Red: diagnóstico", "[argos_net][diagnostics]") {
    argos_net_print_diagnostics();
    TEST_PASS();
}

TEST_CASE("Red: obtener manejador servidor", "[argos_net][server]") {
    httpd_handle_t h = argos_net_get_server();
    TEST_ASSERT_NOT_NULL(h);
    NET_LOG("Server handle OK");
}

TEST_CASE("Red: conteo clientes WS", "[argos_net][websocket]") {
    int count = argos_net_ws_get_client_count();
    TEST_ASSERT_TRUE(count >= 0);
    NET_LOG("WS client count: %d", count);
}

TEST_CASE("Red: desinicialización", "[argos_net][deinit]") {
    esp_err_t ret = argos_net_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_FALSE(argos_net_is_ready());
    NET_LOG("Deinit test OK");
}
