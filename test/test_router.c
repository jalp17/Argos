#include "unity.h"
#include "argos_router.h"
#include "experiment_config.h"
#include "esp_log.h"

static const char *TAG = "test_router";

TEST_CASE("Router: inicialización", "[argos_router][init]") {
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    esp_err_t ret = argos_router_init(&cfg);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(argos_router_is_ready());
    RTR_LOG("Init test OK");
}

TEST_CASE("Router: configuración por defecto", "[argos_router][config]") {
    const argos_experiment_config_t *cfg = argos_router_get_config();
    TEST_ASSERT_NOT_NULL(cfg);
    TEST_ASSERT_EQUAL(4, cfg->num_canales_adc);
    TEST_ASSERT_EQUAL(2, cfg->num_canales_dac);
    TEST_ASSERT_TRUE(cfg->columnas[0].habilitada);
    RTR_LOG("Default config test OK: %s", cfg->nombre);
}

TEST_CASE("Router: inicio y detención", "[argos_router][control]") {
    esp_err_t ret = argos_router_start_experiment();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(EXP_STATE_CORRIENDO, argos_router_get_state());

    ret = argos_router_stop_experiment();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(EXP_STATE_DETENIDO, argos_router_get_state());
    RTR_LOG("Start/stop test OK");
}

TEST_CASE("Router: pausa y reanudación", "[argos_router][control]") {
    argos_router_pause_experiment(true);
    TEST_ASSERT_EQUAL(EXP_STATE_PAUSADO, argos_router_get_state());

    argos_router_pause_experiment(false);
    TEST_ASSERT_EQUAL(EXP_STATE_CORRIENDO, argos_router_get_state());

    argos_router_stop_experiment();
    RTR_LOG("Pause/resume test OK");
}

TEST_CASE("Router: enrutamiento de medición", "[argos_router][route]") {
    argos_measurement_t medicion = {1000, 0, 1.65f};
    int entregados = argos_router_route_measurement(&medicion);
    TEST_ASSERT_TRUE(entregados >= 0);
    RTR_LOG("Route test OK: %d destinos", entregados);
}

TEST_CASE("Router: algoritmos", "[argos_router][algorithm]") {
    float salida = 0;

    /* Probar algoritmo nulo */
    esp_err_t ret = argos_router_run_algorithm_step(1000.0f, &salida);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    RTR_LOG("Algorithm none test OK");

    /* Probar cambio a algoritmo barrido y ejecución */
    argos_experiment_config_t cfg;
    exp_config_apply_template(&cfg, "barrido_dac");
    argos_router_set_config(&cfg);

    ret = argos_router_run_algorithm_step(0, &salida);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    RTR_LOG("Algorithm sweep test OK: salida=%.2f", salida);
}

TEST_CASE("Router: generación CSV", "[argos_router][csv]") {
    const argos_experiment_config_t *cfg = argos_router_get_config();

    char header[256];
    int n = exp_config_generate_csv_header(cfg, header, sizeof(header));
    TEST_ASSERT_TRUE(n > 0);
    RTR_LOG("CSV header: %s", header);

    argos_measurement_t m = {12345, 2, 1.5f};
    char linea[256];
    n = exp_config_measurement_to_csv(cfg, &m, linea, sizeof(linea));
    TEST_ASSERT_TRUE(n > 0);
    RTR_LOG("CSV line: %s", linea);
}

TEST_CASE("Router: plantillas", "[argos_router][templates]") {
    argos_experiment_config_t cfg;

    esp_err_t ret = exp_config_apply_template(&cfg, "barrido_dac");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(EXP_ALG_BARRIDO_DAC, cfg.algoritmo.tipo);
    RTR_LOG("Template barrido_dac: %s", cfg.nombre);

    ret = exp_config_apply_template(&cfg, "lazo_cerrado_pid");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(EXP_ALG_LAZO_CERRADO_PID, cfg.algoritmo.tipo);
    RTR_LOG("Template PID: %s", cfg.nombre);
}

TEST_CASE("Router: listado de plantillas", "[argos_router][templates]") {
    char temps[EXP_CONFIG_TEMPLATE_MAX][EXP_CONFIG_TEMPLATE_NAME_MAX];
    size_t count = 0;
    esp_err_t ret = exp_config_list_templates(temps, EXP_CONFIG_TEMPLATE_MAX, &count);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(count > 0);
    RTR_LOG("Templates: %d encontradas", count);
    for (size_t i = 0; i < count; i++) {
        RTR_LOG("  [%d] %s", i, temps[i]);
    }
}

TEST_CASE("Router: estadísticas", "[argos_router][stats]") {
    argos_router_stats_t stats;
    esp_err_t ret = argos_router_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    RTR_LOG("Stats: %lu mediciones, %lu errores", stats.total_mediciones, stats.errores);
}

TEST_CASE("Router: auto-test", "[argos_router][selftest]") {
    esp_err_t ret = argos_router_self_test();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    RTR_LOG("Self-test OK");
}

TEST_CASE("Router: diagnóstico", "[argos_router][diagnostics]") {
    argos_router_print_diagnostics();
    TEST_PASS();
}

TEST_CASE("Router: desinicialización", "[argos_router][deinit]") {
    argos_router_stop_experiment();
    esp_err_t ret = argos_router_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_FALSE(argos_router_is_ready());
    RTR_LOG("Deinit test OK");
}
