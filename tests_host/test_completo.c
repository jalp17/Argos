/* === Mocks mínimos de tipos ESP-IDF para compilación en host === */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG -2
#define ESP_ERR_NO_MEM -3
#define ESP_ERR_INVALID_STATE -4
#define ESP_ERR_NOT_FOUND -5
#define ESP_ERR_TIMEOUT -6
#define ESP_ERR_NOT_SUPPORTED -7

typedef int32_t adc_channel_t;
typedef int32_t dac_channel_t;
typedef int32_t adc_atten_t;
typedef int32_t adc_bits_width_t;
typedef int32_t gpio_num_t;

#define ADC_CHANNEL_0  0
#define ADC_CHANNEL_3  3
#define ADC_CHANNEL_4  4
#define ADC_CHANNEL_5  5
#define ADC_ATTEN_DB_0  0
#define ADC_ATTEN_DB_2_5  1
#define ADC_ATTEN_DB_6   2
#define ADC_ATTEN_DB_11  3
#define ADC_WIDTH_BIT_9  0
#define ADC_WIDTH_BIT_10 1
#define ADC_WIDTH_BIT_11 2
#define ADC_WIDTH_BIT_12 3
#define DAC_CHAN_0 0
#define DAC_CHAN_1 1
#define GPIO_NUM_2  2
#define GPIO_NUM_18 18
#define GPIO_NUM_19 19

typedef void *QueueHandle_t;
typedef void *SemaphoreHandle_t;
typedef void *TaskHandle_t;

#define pdMS_TO_TICKS(x) (x)
#define portMAX_DELAY ((unsigned)-1)
#define pdTRUE 1
#define pdPASS 1
#define tskNO_AFFINITY (-1)

#define M_PI 3.14159265358979323846

void ESP_LOGI(const char *tag, const char *fmt, ...) { va_list a; va_start(a,fmt); vprintf(fmt,a); va_end(a); printf("\n"); }
void ESP_LOGW(const char *tag, const char *fmt, ...) { va_list a; va_start(a,fmt); vprintf(fmt,a); va_end(a); printf("\n"); }
void ESP_LOGE(const char *tag, const char *fmt, ...) { va_list a; va_start(a,fmt); vprintf(fmt,a); va_end(a); printf("\n"); }

/* ========== TIPOS CORE (replicados de argos_core.h) ========== */
typedef struct {
    uint32_t timestamp;
    float value;
    uint8_t channel;
} argos_measurement_t;

typedef struct {
    uint32_t sample_rate;
    uint8_t adc_attenuation;
    uint8_t dac_resolution;
} argos_config_t;

/* ========== TIPOS EXPERIMENT_CONFIG (replicados exactos) ========== */
#define EXP_CONFIG_MAX_CHANNELS      8
#define EXP_CONFIG_MAX_COLUMNS       16
#define EXP_CONFIG_NAME_MAX          64
#define EXP_CONFIG_DESC_MAX          256
#define EXP_CONFIG_TEMPLATE_MAX      10
#define EXP_CONFIG_TEMPLATE_NAME_MAX 32

typedef enum {
    EXP_TRIGGER_INMEDIATO = 0,
    EXP_TRIGGER_MANUAL = 1,
    EXP_TRIGGER_EXTERNO = 2,
    EXP_TRIGGER_PROGRAMADO = 3
} exp_trigger_mode_t;

typedef enum {
    EXP_SIGNAL_TIPO_ADC = 0,
    EXP_SIGNAL_TIPO_DAC = 1,
    EXP_SIGNAL_TIPO_PWM = 2,
    EXP_SIGNAL_TIPO_GPIO = 3
} exp_signal_type_t;

typedef struct {
    uint8_t indice;
    exp_signal_type_t tipo;
    union {
        adc_channel_t adc_ch;
        dac_channel_t dac_ch;
        uint8_t pwm_ch;
        uint8_t gpio_num;
    };
    bool habilitado;
    char nombre[24];
    char unidad[12];
    float escala;
    float offset;
} exp_channel_config_t;

typedef enum {
    EXP_COL_TIMESTAMP = 0,
    EXP_COL_CHANNEL,
    EXP_COL_VALOR_RAW,
    EXP_COL_VALOR_MV,
    EXP_COL_VALOR_ESCALADO,
    EXP_COL_DAC_OUT,
    EXP_COL_PWM_DUTY
} exp_column_type_t;

typedef struct {
    exp_column_type_t tipo;
    char encabezado[24];
    bool habilitada;
} exp_column_config_t;

typedef enum {
    EXP_ALG_NINGUNO = 0,
    EXP_ALG_BARRIDO_DAC,
    EXP_ALG_LAZO_CERRADO_PID,
    EXP_ALG_RAMPA,
    EXP_ALG_SENO,
    EXP_ALG_CUADRADA,
    EXP_ALG_PERSONALIZADO
} exp_algorithm_type_t;

typedef struct {
    exp_algorithm_type_t tipo;
    float param1;
    float param2;
    float param3;
    float param4;
    char descripcion[128];
} exp_algorithm_config_t;

typedef struct {
    char nombre[EXP_CONFIG_NAME_MAX];
    char descripcion[EXP_CONFIG_DESC_MAX];
    uint32_t version_config;
    exp_channel_config_t canales_adc[EXP_CONFIG_MAX_CHANNELS];
    exp_channel_config_t canales_dac[EXP_CONFIG_MAX_CHANNELS];
    exp_channel_config_t canales_pwm[EXP_CONFIG_MAX_CHANNELS];
    uint8_t num_canales_adc;
    uint8_t num_canales_dac;
    uint8_t num_canales_pwm;
    adc_atten_t atenuacion_adc;
    adc_bits_width_t resolucion_adc;
    uint32_t intervalo_muestreo_ms;
    uint32_t numero_muestras;
    uint32_t duracion_segundos;
    exp_trigger_mode_t modo_disparo;
    uint32_t disparo_retardo_ms;
    exp_column_config_t columnas[EXP_CONFIG_MAX_COLUMNS];
    uint8_t num_columnas;
    exp_algorithm_config_t algoritmo;
    bool incluir_encabezado_csv;
    bool usar_formato_json;
    uint8_t decimales;
    int pin_trigger_in;
    int pin_trigger_out;
    int pin_led_estado;
} argos_experiment_config_t;

typedef enum {
    EXP_STATE_DETENIDO = 0,
    EXP_STATE_CORRIENDO = 1,
    EXP_STATE_PAUSADO = 2,
    EXP_STATE_COMPLETADO = 3,
    EXP_STATE_ERROR = 4
} exp_state_t;

typedef struct {
    char nombre[EXP_CONFIG_TEMPLATE_NAME_MAX];
    char descripcion[128];
    argos_experiment_config_t config;
} exp_template_t;

/* ========== IMPLEMENTACIONES DE LAS FUNCIONES ========== */

void exp_config_init_default(argos_experiment_config_t *config) {
    memset(config, 0, sizeof(argos_experiment_config_t));
    strcpy(config->nombre, "Experimento sin nombre");
    strcpy(config->descripcion, "Configuración por defecto");
    config->version_config = 1;
    config->num_canales_adc = 4;
    const adc_channel_t adcs[] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5};
    const char *nombres_adc[] = {"CH0-GPIO36", "CH1-GPIO39", "CH2-GPIO32", "CH3-GPIO33"};
    for (int i = 0; i < 4; i++) {
        config->canales_adc[i].indice = i;
        config->canales_adc[i].tipo = EXP_SIGNAL_TIPO_ADC;
        config->canales_adc[i].adc_ch = adcs[i];
        config->canales_adc[i].habilitado = true;
        strcpy(config->canales_adc[i].nombre, nombres_adc[i]);
        strcpy(config->canales_adc[i].unidad, "mV");
        config->canales_adc[i].escala = 1.0f;
        config->canales_adc[i].offset = 0.0f;
    }
    config->num_canales_dac = 2;
    const dac_channel_t dacs[] = {DAC_CHAN_0, DAC_CHAN_1};
    const char *nombres_dac[] = {"DAC0-GPIO25", "DAC1-GPIO26"};
    for (int i = 0; i < 2; i++) {
        config->canales_dac[i].indice = i;
        config->canales_dac[i].tipo = EXP_SIGNAL_TIPO_DAC;
        config->canales_dac[i].dac_ch = dacs[i];
        config->canales_dac[i].habilitado = false;
        strcpy(config->canales_dac[i].nombre, nombres_dac[i]);
        strcpy(config->canales_dac[i].unidad, "mV");
    }
    config->num_canales_pwm = 2;
    config->canales_pwm[0].indice = 0;
    config->canales_pwm[0].tipo = EXP_SIGNAL_TIPO_PWM;
    config->canales_pwm[0].habilitado = false;
    strcpy(config->canales_pwm[0].nombre, "PWM0-GPIO18");
    strcpy(config->canales_pwm[0].unidad, "%");
    config->canales_pwm[1].indice = 1;
    config->canales_pwm[1].tipo = EXP_SIGNAL_TIPO_PWM;
    config->canales_pwm[1].habilitado = false;
    strcpy(config->canales_pwm[1].nombre, "PWM1-GPIO19");
    strcpy(config->canales_pwm[1].unidad, "%");
    config->atenuacion_adc = ADC_ATTEN_DB_11;
    config->resolucion_adc = ADC_WIDTH_BIT_12;
    config->intervalo_muestreo_ms = 100;
    config->numero_muestras = 0;
    config->duracion_segundos = 0;
    config->modo_disparo = EXP_TRIGGER_INMEDIATO;
    config->disparo_retardo_ms = 0;
    config->num_columnas = 4;
    config->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    config->columnas[1] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
    config->columnas[2] = (exp_column_config_t){EXP_COL_VALOR_RAW, "valor_crudo", true};
    config->columnas[3] = (exp_column_config_t){EXP_COL_VALOR_MV, "voltaje_mV", true};
    config->algoritmo.tipo = EXP_ALG_NINGUNO;
    strcpy(config->algoritmo.descripcion, "Captura directa sin procesamiento");
    config->incluir_encabezado_csv = true;
    config->usar_formato_json = false;
    config->decimales = 2;
    config->pin_trigger_in = -1;
    config->pin_trigger_out = -1;
    config->pin_led_estado = GPIO_NUM_2;
}

static void plantilla_barrido_dac(argos_experiment_config_t *cfg) {
    strcpy(cfg->nombre, "Barrido DAC");
    strcpy(cfg->descripcion, "Barrido automático de voltaje DAC mientras se lee ADC");
    cfg->num_canales_adc = 1;
    cfg->canales_adc[0].indice = 0;
    cfg->canales_adc[0].tipo = EXP_SIGNAL_TIPO_ADC;
    cfg->canales_adc[0].adc_ch = ADC_CHANNEL_0;
    cfg->canales_adc[0].habilitado = true;
    strcpy(cfg->canales_adc[0].nombre, "Entrada");
    strcpy(cfg->canales_adc[0].unidad, "mV");
    cfg->canales_adc[0].escala = 1.0f;
    cfg->canales_adc[0].offset = 0.0f;
    cfg->num_canales_dac = 1;
    cfg->canales_dac[0].indice = 0;
    cfg->canales_dac[0].tipo = EXP_SIGNAL_TIPO_DAC;
    cfg->canales_dac[0].dac_ch = DAC_CHAN_0;
    cfg->canales_dac[0].habilitado = true;
    strcpy(cfg->canales_dac[0].nombre, "Barrido");
    strcpy(cfg->canales_dac[0].unidad, "mV");
    cfg->intervalo_muestreo_ms = 100;
    cfg->numero_muestras = 256;
    cfg->atenuacion_adc = ADC_ATTEN_DB_11;
    cfg->algoritmo.tipo = EXP_ALG_BARRIDO_DAC;
    cfg->algoritmo.param1 = 3300.0f;
    cfg->algoritmo.param2 = 256.0f;
    strcpy(cfg->algoritmo.descripcion, "Barre DAC de 0 a 3300 mV en 256 pasos");
    cfg->num_columnas = 5;
    cfg->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    cfg->columnas[1] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
    cfg->columnas[2] = (exp_column_config_t){EXP_COL_VALOR_MV, "entrada_mV", true};
    cfg->columnas[3] = (exp_column_config_t){EXP_COL_DAC_OUT, "salida_mV", true};
    cfg->columnas[4] = (exp_column_config_t){EXP_COL_VALOR_ESCALADO, "escalado", false};
}

static void plantilla_lazo_cerrado_pid(argos_experiment_config_t *cfg) {
    strcpy(cfg->nombre, "Lazo Cerrado PID");
    strcpy(cfg->descripcion, "Control PID en lazo cerrado con realimentación ADC");
    cfg->num_canales_adc = 1;
    cfg->canales_adc[0].indice = 0;
    cfg->canales_adc[0].tipo = EXP_SIGNAL_TIPO_ADC;
    cfg->canales_adc[0].adc_ch = ADC_CHANNEL_0;
    cfg->canales_adc[0].habilitado = true;
    strcpy(cfg->canales_adc[0].nombre, "Sensor");
    strcpy(cfg->canales_adc[0].unidad, "mV");
    cfg->num_canales_dac = 1;
    cfg->canales_dac[0].indice = 0;
    cfg->canales_dac[0].tipo = EXP_SIGNAL_TIPO_DAC;
    cfg->canales_dac[0].dac_ch = DAC_CHAN_0;
    cfg->canales_dac[0].habilitado = true;
    strcpy(cfg->canales_dac[0].nombre, "Control");
    strcpy(cfg->canales_dac[0].unidad, "mV");
    cfg->intervalo_muestreo_ms = 50;
    cfg->numero_muestras = 0;
    cfg->atenuacion_adc = ADC_ATTEN_DB_11;
    cfg->algoritmo.tipo = EXP_ALG_LAZO_CERRADO_PID;
    cfg->algoritmo.param1 = 1.0f;
    cfg->algoritmo.param2 = 0.1f;
    cfg->algoritmo.param3 = 0.05f;
    cfg->algoritmo.param4 = 1500.0f;
    strcpy(cfg->algoritmo.descripcion, "PID: Kp=1.0, Ki=0.1, Kd=0.05, SP=1500mV");
    cfg->num_columnas = 6;
    cfg->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    cfg->columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "medicion_mV", true};
    cfg->columnas[2] = (exp_column_config_t){EXP_COL_DAC_OUT, "control_mV", true};
    cfg->columnas[3] = (exp_column_config_t){EXP_COL_VALOR_ESCALADO, "setpoint_mV", true};
    cfg->columnas[4] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", false};
    cfg->columnas[5] = (exp_column_config_t){EXP_COL_VALOR_RAW, "raw_adc", false};
}

static void plantilla_rampa(argos_experiment_config_t *cfg) {
    strcpy(cfg->nombre, "Generación de Rampa");
    strcpy(cfg->descripcion, "Genera una rampa lineal en el DAC mientras registra ADC");
    cfg->algoritmo.tipo = EXP_ALG_RAMPA;
    cfg->algoritmo.param1 = 3300.0f;
    cfg->algoritmo.param2 = 500.0f;
    cfg->intervalo_muestreo_ms = 10;
    cfg->num_columnas = 4;
    cfg->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    cfg->columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "lectura_mV", true};
    cfg->columnas[2] = (exp_column_config_t){EXP_COL_DAC_OUT, "rampa_mV", true};
    cfg->columnas[3] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
}

static void plantilla_seno(argos_experiment_config_t *cfg) {
    strcpy(cfg->nombre, "Onda Senoidal");
    strcpy(cfg->descripcion, "Genera una onda senoidal en el DAC");
    cfg->algoritmo.tipo = EXP_ALG_SENO;
    cfg->algoritmo.param1 = 1650.0f;
    cfg->algoritmo.param2 = 1650.0f;
    cfg->algoritmo.param3 = 1.0f;
    cfg->intervalo_muestreo_ms = 20;
    cfg->numero_muestras = 500;
    cfg->num_columnas = 4;
    cfg->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    cfg->columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "lectura_mV", true};
    cfg->columnas[2] = (exp_column_config_t){EXP_COL_DAC_OUT, "seno_mV", true};
    cfg->columnas[3] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
}

static void plantilla_cuadrada(argos_experiment_config_t *cfg) {
    strcpy(cfg->nombre, "Onda Cuadrada");
    strcpy(cfg->descripcion, "Genera una onda cuadrada en el DAC");
    cfg->algoritmo.tipo = EXP_ALG_CUADRADA;
    cfg->algoritmo.param1 = 3300.0f;
    cfg->algoritmo.param2 = 0.0f;
    cfg->algoritmo.param3 = 500.0f;
    cfg->algoritmo.param4 = 50.0f;
    cfg->intervalo_muestreo_ms = 10;
    cfg->numero_muestras = 1000;
    cfg->num_columnas = 4;
    cfg->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    cfg->columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "lectura_mV", true};
    cfg->columnas[2] = (exp_column_config_t){EXP_COL_DAC_OUT, "cuadrada_mV", true};
    cfg->columnas[3] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
}

esp_err_t exp_config_apply_template(argos_experiment_config_t *config, const char *template) {
    if (config == NULL || template == NULL) return ESP_ERR_INVALID_ARG;
    exp_config_init_default(config);
    if (strcmp(template, "default") == 0) return ESP_OK;
    if (strcmp(template, "barrido_dac") == 0) { plantilla_barrido_dac(config); return ESP_OK; }
    if (strcmp(template, "lazo_cerrado_pid") == 0) { plantilla_lazo_cerrado_pid(config); return ESP_OK; }
    if (strcmp(template, "rampa") == 0) { plantilla_rampa(config); return ESP_OK; }
    if (strcmp(template, "seno") == 0) { plantilla_seno(config); return ESP_OK; }
    if (strcmp(template, "cuadrada") == 0) { plantilla_cuadrada(config); return ESP_OK; }
    return ESP_ERR_NOT_FOUND;
}

int exp_config_generate_csv_header(const argos_experiment_config_t *config, char *buffer, size_t buffer_size) {
    int n = 0; bool primera = true;
    for (uint8_t i = 0; i < config->num_columnas; i++) {
        if (!config->columnas[i].habilitada) continue;
        if (!primera) n += snprintf(buffer + n, buffer_size - n, ",");
        n += snprintf(buffer + n, buffer_size - n, "%s", config->columnas[i].encabezado);
        primera = false;
    }
    n += snprintf(buffer + n, buffer_size - n, "\n");
    return n;
}

int exp_config_measurement_to_csv(const argos_experiment_config_t *config,
                                   const argos_measurement_t *medicion,
                                   char *buffer, size_t buffer_size) {
    int n = 0; bool primera = true;
    for (uint8_t i = 0; i < config->num_columnas; i++) {
        if (!config->columnas[i].habilitada) continue;
        if (!primera) n += snprintf(buffer + n, buffer_size - n, ",");
        switch (config->columnas[i].tipo) {
            case EXP_COL_TIMESTAMP:
                n += snprintf(buffer + n, buffer_size - n, "%lu", (unsigned long)medicion->timestamp);
                break;
            case EXP_COL_CHANNEL:
                n += snprintf(buffer + n, buffer_size - n, "%d", medicion->channel);
                break;
            case EXP_COL_VALOR_RAW:
                n += snprintf(buffer + n, buffer_size - n, "%d", (int)(medicion->value * 1000));
                break;
            case EXP_COL_VALOR_MV:
                n += snprintf(buffer + n, buffer_size - n, "%.*f", config->decimales, medicion->value * 1000.0f);
                break;
            case EXP_COL_VALOR_ESCALADO: {
                if (medicion->channel < config->num_canales_adc) {
                    float escalado = (medicion->value * config->canales_adc[medicion->channel].escala)
                                     + config->canales_adc[medicion->channel].offset;
                    n += snprintf(buffer + n, buffer_size - n, "%.*f", config->decimales, escalado);
                } else {
                    n += snprintf(buffer + n, buffer_size - n, "%.*f", config->decimales, medicion->value);
                }
                break;
            }
            case EXP_COL_DAC_OUT:
                n += snprintf(buffer + n, buffer_size - n, "0");
                break;
            case EXP_COL_PWM_DUTY:
                n += snprintf(buffer + n, buffer_size - n, "0");
                break;
        }
        primera = false;
    }
    n += snprintf(buffer + n, buffer_size - n, "\n");
    return n;
}

int exp_config_to_json(const argos_experiment_config_t *config, char *buffer, size_t buffer_size) {
    return snprintf(buffer, buffer_size,
        "{\"nombre\":\"%s\",\"descripcion\":\"%s\",\"intervalo_ms\":%lu,\"muestras\":%lu,\"algoritmo\":%d}",
        config->nombre, config->descripcion,
        (unsigned long)config->intervalo_muestreo_ms,
        (unsigned long)config->numero_muestras,
        config->algoritmo.tipo);
}

esp_err_t exp_config_from_json(argos_experiment_config_t *config, const char *json) {
    (void)config; (void)json;
    return ESP_ERR_NOT_SUPPORTED;
}

/* ==================== ALGORITMOS ==================== */

static int s_algo_paso = 0;

static esp_err_t algo_barrido(float entrada, float *salida, const argos_experiment_config_t *cfg) {
    (void)entrada;
    float v_max = cfg->algoritmo.param1;
    float pasos = cfg->algoritmo.param2;
    if (pasos <= 0) pasos = 256;
    if (s_algo_paso >= (int)pasos) { *salida = 0; s_algo_paso = 0; return ESP_FAIL; }
    *salida = ((float)s_algo_paso / pasos) * v_max;
    s_algo_paso++;
    return ESP_OK;
}

static esp_err_t algo_pid(float entrada, float *salida, const argos_experiment_config_t *cfg) {
    static float integral = 0, error_anterior = 0;
    float kp = cfg->algoritmo.param1, ki = cfg->algoritmo.param2;
    float kd = cfg->algoritmo.param3, setpoint = cfg->algoritmo.param4;
    float error = setpoint - entrada;
    integral += error;
    float derivada = error - error_anterior;
    error_anterior = error;
    float control = (kp * error) + (ki * integral) + (kd * derivada);
    if (control < 0) control = 0;
    if (control > 3300) control = 3300;
    *salida = control;
    return ESP_OK;
}

static esp_err_t algo_rampa(float entrada, float *salida, const argos_experiment_config_t *cfg) {
    (void)entrada;
    float v_max = cfg->algoritmo.param1;
    float duracion = cfg->algoritmo.param2;
    if (duracion <= 0) duracion = 500;
    float progreso = (float)(s_algo_paso * cfg->intervalo_muestreo_ms) / duracion;
    if (progreso >= 1.0f) { s_algo_paso = 0; progreso = 0; }
    *salida = progreso * v_max;
    s_algo_paso++;
    return ESP_OK;
}

static esp_err_t algo_seno(float entrada, float *salida, const argos_experiment_config_t *cfg) {
    (void)entrada;
    float amplitud = cfg->algoritmo.param1, offset = cfg->algoritmo.param2;
    float freq = cfg->algoritmo.param3; if (freq <= 0) freq = 1.0f;
    float t = (float)(s_algo_paso * cfg->intervalo_muestreo_ms) / 1000.0f;
    *salida = offset + amplitud * sinf(2.0f * M_PI * freq * t);
    s_algo_paso++;
    return ESP_OK;
}

static esp_err_t algo_cuadrada(float entrada, float *salida, const argos_experiment_config_t *cfg) {
    (void)entrada;
    float alto = cfg->algoritmo.param1, bajo = cfg->algoritmo.param2;
    float periodo_ms = cfg->algoritmo.param3, duty = cfg->algoritmo.param4;
    if (periodo_ms <= 0) periodo_ms = 500;
    float t = (float)(s_algo_paso * cfg->intervalo_muestreo_ms);
    float ciclo = fmodf(t, periodo_ms);
    *salida = (ciclo < periodo_ms * (duty / 100.0f)) ? alto : bajo;
    s_algo_paso++;
    return ESP_OK;
}

/* ==================== SISTEMA DE PRUEBAS ==================== */
static int tests_pasados = 0, tests_fallados = 0, tests_total = 0;

#define ASSERT(cond, msg) do { \
    tests_total++; \
    if (!(cond)) { printf("  FALLÓ: %s\n", msg); tests_fallados++; } \
    else { printf("  OK: %s\n", msg); tests_pasados++; } \
} while(0)

#define ASSERT_EQ(a,b,msg) do { \
    tests_total++; \
    if ((a) != (b)) { printf("  FALLÓ: %s (esperado=%d, obtenido=%d)\n", msg, (int)(b), (int)(a)); tests_fallados++; } \
    else { printf("  OK: %s\n", msg); tests_pasados++; } \
} while(0)

#define ASSERT_STR(a,b,msg) do { \
    tests_total++; \
    if (strcmp((a),(b)) != 0) { printf("  FALLÓ: %s\n    esperado: '%s'\n    obtenido: '%s'\n", msg, (b), (a)); tests_fallados++; } \
    else { printf("  OK: %s\n", msg); tests_pasados++; } \
} while(0)

#define ASSERT_NEAR(a,b,tol,msg) do { \
    tests_total++; \
    if (fabsf((a)-(b)) > (tol)) { \
        printf("  FALLÓ: %s (esperado=%.2f, obtenido=%.2f, tol=%.2f)\n", msg, (float)(b), (float)(a), (float)(tol)); \
        tests_fallados++; \
    } else { printf("  OK: %s\n", msg); tests_pasados++; } \
} while(0)

void test_config_inicializacion(void) {
    printf("\n--- Config: Inicialización por defecto ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    ASSERT_STR(cfg.nombre, "Experimento sin nombre", "Nombre por defecto");
    ASSERT_EQ(cfg.num_canales_adc, 4, "4 canales ADC");
    ASSERT_EQ(cfg.num_canales_dac, 2, "2 canales DAC");
    ASSERT_EQ(cfg.num_canales_pwm, 2, "2 canales PWM");
    ASSERT_EQ(cfg.intervalo_muestreo_ms, 100, "Intervalo 100ms");
    ASSERT_EQ(cfg.numero_muestras, 0, "Muestras 0=continuo");
    ASSERT_EQ(cfg.decimales, 2, "Decimales=2");
    ASSERT_EQ(cfg.algoritmo.tipo, EXP_ALG_NINGUNO, "Algoritmo ninguno");
    ASSERT(cfg.canales_adc[0].habilitado, "Canal ADC 0 habilitado");
    ASSERT_STR(cfg.canales_adc[0].nombre, "CH0-GPIO36", "Nombre canal ADC 0");
    ASSERT_STR(cfg.canales_adc[0].unidad, "mV", "Unidad canal ADC 0");
}

void test_config_plantilla_barrido(void) {
    printf("\n--- Config: Plantilla barrido DAC ---\n");
    argos_experiment_config_t cfg;
    esp_err_t ret = exp_config_apply_template(&cfg, "barrido_dac");
    ASSERT_EQ(ret, ESP_OK, "Aplicar plantilla barrido_dac");
    ASSERT_STR(cfg.nombre, "Barrido DAC", "Nombre plantilla");
    ASSERT_EQ(cfg.algoritmo.tipo, EXP_ALG_BARRIDO_DAC, "Algoritmo barrido");
    ASSERT_EQ(cfg.numero_muestras, 256, "256 muestras");
    ASSERT_EQ(cfg.intervalo_muestreo_ms, 100, "Intervalo 100ms");
    ASSERT_EQ(cfg.num_canales_adc, 1, "1 canal ADC");
    ASSERT(cfg.canales_dac[0].habilitado, "DAC habilitado");
    ASSERT_EQ(cfg.num_columnas, 5, "5 columnas");
    ASSERT_STR(cfg.columnas[0].encabezado, "tiempo_ms", "Col 0=tiempo_ms");
    ASSERT_STR(cfg.columnas[3].encabezado, "salida_mV", "Col 3=salida_mV");
}

void test_config_plantilla_no_valida(void) {
    printf("\n--- Config: Plantilla no válida ---\n");
    argos_experiment_config_t cfg;
    esp_err_t ret = exp_config_apply_template(&cfg, "no_existe");
    ASSERT_EQ(ret, ESP_ERR_NOT_FOUND, "Plantilla inexistente devuelve error");
}

void test_csv_encabezado(void) {
    printf("\n--- CSV: Generación de encabezado ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    char buffer[256];
    exp_config_generate_csv_header(&cfg, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "tiempo_ms,canal,valor_crudo,voltaje_mV\n", "Encabezado por defecto");

    cfg.num_columnas = 3;
    cfg.columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "t_ms", true};
    cfg.columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "mV", true};
    cfg.columnas[2] = (exp_column_config_t){EXP_COL_CHANNEL, "ch", false};
    exp_config_generate_csv_header(&cfg, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "t_ms,mV\n", "Encabezado personalizado (col 2 oculta)");
}

void test_csv_linea_medicion(void) {
    printf("\n--- CSV: Línea de medición ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    argos_measurement_t m = {12345, 1.5f, 2};
    char buffer[256];
    exp_config_measurement_to_csv(&cfg, &m, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "12345,2,1500,1500.00\n",
              "CSV: timestamp=12345, ch=2, raw=1500, mV=1500.00");
}

void test_csv_escala(void) {
    printf("\n--- CSV: Factor de escala ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.canales_adc[0].escala = 10.0f;
    cfg.canales_adc[0].offset = 5.0f;
    cfg.num_columnas = 2;
    cfg.columnas[0] = (exp_column_config_t){EXP_COL_VALOR_ESCALADO, "escalado", true};
    cfg.columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "mV", true};

    argos_measurement_t m = {0, 2.5f, 0};
    char buffer[256];
    exp_config_measurement_to_csv(&cfg, &m, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "30.00,2500.00\n", "Escala 10x + offset 5");
}

void test_json_serializacion(void) {
    printf("\n--- JSON: Serialización ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    strcpy(cfg.nombre, "Test");
    cfg.intervalo_muestreo_ms = 500;
    cfg.numero_muestras = 100;
    char buffer[512];
    int n = exp_config_to_json(&cfg, buffer, sizeof(buffer));
    ASSERT(n > 0, "JSON generado");
    ASSERT(buffer[0] == '{', "JSON empieza con {");
}

void test_algoritmo_barrido(void) {
    printf("\n--- Algoritmo: Barrido DAC ---\n");
    s_algo_paso = 0;
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.algoritmo.tipo = EXP_ALG_BARRIDO_DAC;
    cfg.algoritmo.param1 = 3300.0f;
    cfg.algoritmo.param2 = 10.0f;

    float salida = 0;
    esp_err_t ret;

    ret = algo_barrido(0, &salida, &cfg);
    ASSERT_EQ(ret, ESP_OK, "Barrido paso 0 OK");
    ASSERT_NEAR(salida, 0.0f, 0.1f, "Barrido paso 0 = 0");

    ret = algo_barrido(0, &salida, &cfg);
    ASSERT_NEAR(salida, 330.0f, 0.1f, "Barrido paso 1 = 330");

    for (int i = 2; i <= 9; i++) algo_barrido(0, &salida, &cfg);
    ASSERT_NEAR(salida, 2970.0f, 0.1f, "Barrido paso 9 = 2970");

    ret = algo_barrido(0, &salida, &cfg);
    ASSERT_EQ(ret, ESP_FAIL, "Barrido paso 10 = terminado");
    ASSERT_NEAR(salida, 0.0f, 0.1f, "Barrido post-fin = 0");
}

void test_algoritmo_pid(void) {
    printf("\n--- Algoritmo: PID ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.algoritmo.tipo = EXP_ALG_LAZO_CERRADO_PID;
    cfg.algoritmo.param1 = 1.0f;
    cfg.algoritmo.param2 = 0.1f;
    cfg.algoritmo.param3 = 0.0f;
    cfg.algoritmo.param4 = 1500.0f;

    float salida = 0;
    esp_err_t ret = algo_pid(1000.0f, &salida, &cfg);
    ASSERT_EQ(ret, ESP_OK, "PID ejecutado");
    ASSERT_NEAR(salida, 550.0f, 10.0f, "PID salida ~550 con Kp=1, Ki=0.1");

    ret = algo_pid(1000.0f, &salida, &cfg);
    ASSERT_NEAR(salida, 600.0f, 10.0f, "PID segunda iteración ~600");

    ret = algo_pid(1500.0f, &salida, &cfg);
    ASSERT_NEAR(salida, 100.0f, 10.0f, "PID error=0, integral residual");
}

void test_algoritmo_rampa(void) {
    printf("\n--- Algoritmo: Rampa ---\n");
    s_algo_paso = 0;
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.algoritmo.tipo = EXP_ALG_RAMPA;
    cfg.algoritmo.param1 = 3300.0f;
    cfg.algoritmo.param2 = 200.0f;
    cfg.intervalo_muestreo_ms = 100;

    float salida = 0;

    algo_rampa(0, &salida, &cfg);
    ASSERT_NEAR(salida, 0.0f, 0.1f, "Rampa t=0ms = 0");

    algo_rampa(0, &salida, &cfg);
    ASSERT_NEAR(salida, 1650.0f, 1.0f, "Rampa t=100ms = 1650 (50%)");

    algo_rampa(0, &salida, &cfg);
    ASSERT_NEAR(salida, 0.0f, 0.1f, "Rampa t=200ms = 0 (reset al completar)");
}

void test_algoritmo_seno(void) {
    printf("\n--- Algoritmo: Seno ---\n");
    s_algo_paso = 0;
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.algoritmo.tipo = EXP_ALG_SENO;
    cfg.algoritmo.param1 = 1000.0f;
    cfg.algoritmo.param2 = 1500.0f;
    cfg.algoritmo.param3 = 1.0f;
    cfg.intervalo_muestreo_ms = 250;

    float salida = 0;
    algo_seno(0, &salida, &cfg);
    ASSERT_NEAR(salida, 1500.0f, 1.0f, "Seno t=0: 1500");

    algo_seno(0, &salida, &cfg);
    ASSERT_NEAR(salida, 2500.0f, 100.0f, "Seno t=250ms: ~2500");

    algo_seno(0, &salida, &cfg);
    ASSERT_NEAR(salida, 1500.0f, 100.0f, "Seno t=500ms: ~1500");

    algo_seno(0, &salida, &cfg);
    ASSERT_NEAR(salida, 500.0f, 100.0f, "Seno t=750ms: ~500");
}

void test_algoritmo_cuadrada(void) {
    printf("\n--- Algoritmo: Cuadrada ---\n");
    s_algo_paso = 0;
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.algoritmo.tipo = EXP_ALG_CUADRADA;
    cfg.algoritmo.param1 = 3300.0f;
    cfg.algoritmo.param2 = 0.0f;
    cfg.algoritmo.param3 = 200.0f;
    cfg.algoritmo.param4 = 50.0f;
    cfg.intervalo_muestreo_ms = 100;

    float salida = 0;
    algo_cuadrada(0, &salida, &cfg);
    ASSERT_NEAR(salida, 3300.0f, 1.0f, "Cuadrada t=0: alto (0 < 100)");

    algo_cuadrada(0, &salida, &cfg);
    ASSERT_NEAR(salida, 0.0f, 1.0f, "Cuadrada t=100: bajo (100 < 100 = false)");

    algo_cuadrada(0, &salida, &cfg);
    ASSERT_NEAR(salida, 3300.0f, 1.0f, "Cuadrada t=200: alto (reset ciclo)");
}

void test_limites(void) {
    printf("\n--- Límites y valores extremos ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);

    cfg.num_columnas = 3;
    cfg.columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "t", false};
    cfg.columnas[1] = (exp_column_config_t){EXP_COL_CHANNEL, "ch", false};
    cfg.columnas[2] = (exp_column_config_t){EXP_COL_VALOR_MV, "mV", false};
    char buffer[64];
    exp_config_generate_csv_header(&cfg, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "\n", "Todas columnas ocultas");

    cfg.num_columnas = 0;
    exp_config_generate_csv_header(&cfg, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "\n", "Sin columnas");

    argos_measurement_t m = {0xFFFFFFFF, 3.3f, 255};
    cfg.num_columnas = 4;
    cfg.columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "t", true};
    cfg.columnas[1] = (exp_column_config_t){EXP_COL_CHANNEL, "ch", true};
    cfg.columnas[2] = (exp_column_config_t){EXP_COL_VALOR_RAW, "raw", true};
    cfg.columnas[3] = (exp_column_config_t){EXP_COL_VALOR_MV, "mv", true};
    exp_config_measurement_to_csv(&cfg, &m, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "4294967295,255,3300,3300.00\n", "Valores extremos");
}

void test_plantillas_disponibles(void) {
    printf("\n--- Plantillas: Listado y verificación ---\n");
    argos_experiment_config_t cfg;
    esp_err_t ret;

    ret = exp_config_apply_template(&cfg, "default");
    ASSERT_EQ(ret, ESP_OK, "Template 'default'");
    ASSERT_EQ(cfg.algoritmo.tipo, EXP_ALG_NINGUNO, "Default = ninguno");

    ret = exp_config_apply_template(&cfg, "barrido_dac");
    ASSERT_EQ(ret, ESP_OK, "Template 'barrido_dac'");
    ASSERT_EQ(cfg.algoritmo.tipo, EXP_ALG_BARRIDO_DAC, "Barrido_dac correcto");
    ASSERT_EQ(cfg.numero_muestras, 256, "barrido: 256 muestras");
    ASSERT_EQ(cfg.intervalo_muestreo_ms, 100, "barrido: 100ms");

    ret = exp_config_apply_template(&cfg, "lazo_cerrado_pid");
    ASSERT_EQ(ret, ESP_OK, "Template 'lazo_cerrado_pid'");
    ASSERT_EQ(cfg.algoritmo.tipo, EXP_ALG_LAZO_CERRADO_PID, "PID correcto");
    ASSERT_EQ(cfg.algoritmo.param1, 1.0f, "PID Kp=1.0");
    ASSERT_EQ(cfg.algoritmo.param4, 1500.0f, "PID SP=1500");
    ASSERT_STR(cfg.nombre, "Lazo Cerrado PID", "PID nombre");
    ASSERT_EQ(cfg.num_columnas, 6, "PID: 6 columnas");
    ASSERT_STR(cfg.columnas[2].encabezado, "control_mV", "PID col 2=control_mV");

    ret = exp_config_apply_template(&cfg, "rampa");
    ASSERT_EQ(ret, ESP_OK, "Template 'rampa'");
    ASSERT_EQ(cfg.algoritmo.tipo, EXP_ALG_RAMPA, "Rampa correcto");
    ASSERT_EQ(cfg.algoritmo.param1, 3300.0f, "Rampa Vmax=3300");
    ASSERT_EQ(cfg.intervalo_muestreo_ms, 10, "Rampa: 10ms");
    ASSERT_EQ(cfg.num_columnas, 4, "Rampa: 4 columnas");
    ASSERT_STR(cfg.columnas[2].encabezado, "rampa_mV", "Rampa col 2=rampa_mV");

    ret = exp_config_apply_template(&cfg, "seno");
    ASSERT_EQ(ret, ESP_OK, "Template 'seno'");
    ASSERT_EQ(cfg.algoritmo.tipo, EXP_ALG_SENO, "Seno correcto");
    ASSERT_EQ(cfg.algoritmo.param1, 1650.0f, "Seno amplitud=1650");
    ASSERT_EQ(cfg.algoritmo.param2, 1650.0f, "Seno offset=1650");
    ASSERT_EQ(cfg.numero_muestras, 500, "Seno: 500 muestras");
    ASSERT_EQ(cfg.num_columnas, 4, "Seno: 4 columnas");
    ASSERT_STR(cfg.columnas[2].encabezado, "seno_mV", "Seno col 2=seno_mV");

    ret = exp_config_apply_template(&cfg, "cuadrada");
    ASSERT_EQ(ret, ESP_OK, "Template 'cuadrada'");
    ASSERT_EQ(cfg.algoritmo.tipo, EXP_ALG_CUADRADA, "Cuadrada correcto");
    ASSERT_EQ(cfg.algoritmo.param1, 3300.0f, "Cuadrada alto=3300");
    ASSERT_EQ(cfg.algoritmo.param2, 0.0f, "Cuadrada bajo=0");
    ASSERT_EQ(cfg.algoritmo.param3, 500.0f, "Cuadrada periodo=500");
    ASSERT_EQ(cfg.numero_muestras, 1000, "Cuadrada: 1000 muestras");
    ASSERT_EQ(cfg.num_columnas, 4, "Cuadrada: 4 columnas");
    ASSERT_STR(cfg.columnas[2].encabezado, "cuadrada_mV", "Cuadrada col 2=cuadrada_mV");
}

void test_parametros_nulos(void) {
    printf("\n--- Robustez: Parámetros nulos ---\n");
    esp_err_t ret = exp_config_apply_template(NULL, "default");
    ASSERT_EQ(ret, ESP_ERR_INVALID_ARG, "apply_template(NULL,.)");

    ret = exp_config_apply_template(NULL, NULL);
    ASSERT_EQ(ret, ESP_ERR_INVALID_ARG, "apply_template(NULL,NULL)");
}

void test_column_order_reordering(void) {
    printf("\n--- CSV: Reordenamiento de columnas ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.num_columnas = 3;
    cfg.columnas[0] = (exp_column_config_t){EXP_COL_VALOR_MV, "mV", true};
    cfg.columnas[1] = (exp_column_config_t){EXP_COL_TIMESTAMP, "t", true};
    cfg.columnas[2] = (exp_column_config_t){EXP_COL_CHANNEL, "ch", true};

    char buffer[64];
    exp_config_generate_csv_header(&cfg, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "mV,t,ch\n", "Columnas: mV,t,ch");

    argos_measurement_t m = {999, 1.2f, 3};
    exp_config_measurement_to_csv(&cfg, &m, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "1200.00,999,3\n", "Datos reordenados");
}

void test_csv_dac_pwm_columns(void) {
    printf("\n--- CSV: Columnas DAC y PWM ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.num_columnas = 2;
    cfg.columnas[0] = (exp_column_config_t){EXP_COL_DAC_OUT, "dac", true};
    cfg.columnas[1] = (exp_column_config_t){EXP_COL_PWM_DUTY, "pwm", true};

    char buffer[64];
    argos_measurement_t m = {0, 0, 0};
    exp_config_measurement_to_csv(&cfg, &m, buffer, sizeof(buffer));
    ASSERT_STR(buffer, "0,0\n", "DAC y PWM = 0");
}

void test_parametros_algoritmo(void) {
    printf("\n--- Robustez: Parámetros de algoritmo inválidos ---\n");
    argos_experiment_config_t cfg;
    exp_config_init_default(&cfg);
    cfg.intervalo_muestreo_ms = 100;

    float salida = 999;

    /* Barrido con pasos=0 */
    cfg.algoritmo.tipo = EXP_ALG_BARRIDO_DAC;
    cfg.algoritmo.param1 = 3300;
    cfg.algoritmo.param2 = 0;
    s_algo_paso = 0;
    algo_barrido(0, &salida, &cfg);
    ASSERT_NEAR(salida, 0.0f, 0.1f, "Barrido param2=0 default a 256, paso 0 = 0");

    /* Rampa con duracion=0 */
    cfg.algoritmo.tipo = EXP_ALG_RAMPA;
    cfg.algoritmo.param1 = 1000;
    cfg.algoritmo.param2 = 0;
    s_algo_paso = 0;
    algo_rampa(0, &salida, &cfg);
    ASSERT_NEAR(salida, 0.0f, 0.1f, "Rampa duracion=0 default 500ms, paso 0 = 0");

    /* Seno con freq=0 */
    cfg.algoritmo.tipo = EXP_ALG_SENO;
    cfg.algoritmo.param3 = 0;
    s_algo_paso = 0;
    algo_seno(0, &salida, &cfg);
    ASSERT_NEAR(salida, 0.0f, 0.1f, "Seno freq=0 default 1 Hz, t=0 = 0");

    /* Cuadrada con periodo=0 */
    cfg.algoritmo.tipo = EXP_ALG_CUADRADA;
    cfg.algoritmo.param1 = 100;
    cfg.algoritmo.param2 = 0;
    cfg.algoritmo.param3 = 0;
    cfg.algoritmo.param4 = 50;
    s_algo_paso = 0;
    algo_cuadrada(0, &salida, &cfg);
    ASSERT_NEAR(salida, 100.0f, 0.1f, "Cuadrada periodo=0 default 500ms, ciclo=0 < 250 = true");
}

int main(void) {
    printf("========================================\n");
    printf("  Argos - Suite de Pruebas sin Hardware\n");
    printf("========================================\n");

    test_config_inicializacion();
    test_config_plantilla_barrido();
    test_config_plantilla_no_valida();
    test_csv_encabezado();
    test_csv_linea_medicion();
    test_csv_escala();
    test_json_serializacion();
    test_algoritmo_barrido();
    test_algoritmo_pid();
    test_algoritmo_rampa();
    test_algoritmo_seno();
    test_algoritmo_cuadrada();
    test_limites();
    test_plantillas_disponibles();
    test_parametros_nulos();
    test_column_order_reordering();
    test_csv_dac_pwm_columns();
    test_parametros_algoritmo();

    printf("\n========================================\n");
    printf("  Resultados: %d/%d pasados, %d fallados\n",
           tests_pasados, tests_total, tests_fallados);
    printf("========================================\n");

    return tests_fallados > 0 ? 1 : 0;
}
