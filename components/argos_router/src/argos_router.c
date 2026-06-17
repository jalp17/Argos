#include "argos_router.h"
#include "argos_core.h"
#include "argos_net.h"
#include "experiment_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

static const char *TAG = "argos_router";

/* ==================== MACROS DE DEPURACIÓN ==================== */
#define RTR_LOG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define RTR_ERR(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define RTR_WARN(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)

/* ==================== CONSTANTES INTERNAS ==================== */
#define RUTA_TEMPLATES "/data/templates"
#define EXP_FILE_EXT   ".json"

/* ==================== ESTADO INTERNO ==================== */
static bool s_inicializado = false;
static argos_experiment_config_t s_config;
static exp_state_t s_estado = EXP_STATE_DETENIDO;
static TaskHandle_t s_adq_task = NULL;
static QueueHandle_t s_data_queue = NULL;
static QueueHandle_t s_control_queue = NULL;
static SemaphoreHandle_t s_config_mutex = NULL;
static SemaphoreHandle_t s_state_mutex = NULL;

static uint32_t s_total_mediciones = 0;
static uint32_t s_errores = 0;
static int64_t s_tiempo_inicio = 0;
static float s_algo_acumulador = 0.0f;
static uint32_t s_algo_paso = 0;
static bool s_algo_terminado = false;

/* ==================== PLANTILLAS PREDEFINIDAS ==================== */

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
    cfg->algoritmo.param1 = 3300.0f;  /* Voltaje máximo */
    cfg->algoritmo.param2 = 256.0f;   /* Número de pasos */
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
    cfg->numero_muestras = 0; /* Continuo */
    cfg->atenuacion_adc = ADC_ATTEN_DB_11;

    cfg->algoritmo.tipo = EXP_ALG_LAZO_CERRADO_PID;
    cfg->algoritmo.param1 = 1.0f;   /* Kp */
    cfg->algoritmo.param2 = 0.1f;   /* Ki */
    cfg->algoritmo.param3 = 0.05f;  /* Kd */
    cfg->algoritmo.param4 = 1500.0f; /* Setpoint (mV) */
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
    memcpy(cfg, &s_config, sizeof(argos_experiment_config_t));
    strcpy(cfg->nombre, "Generación de Rampa");
    strcpy(cfg->descripcion, "Genera una rampa lineal en el DAC mientras registra ADC");

    cfg->algoritmo.tipo = EXP_ALG_RAMPA;
    cfg->algoritmo.param1 = 3300.0f; /* Voltaje máximo */
    cfg->algoritmo.param2 = 500.0f;  /* Duración del ciclo (ms) */
    cfg->intervalo_muestreo_ms = 10;

    cfg->num_columnas = 4;
    cfg->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    cfg->columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "lectura_mV", true};
    cfg->columnas[2] = (exp_column_config_t){EXP_COL_DAC_OUT, "rampa_mV", true};
    cfg->columnas[3] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
}

static void plantilla_seno(argos_experiment_config_t *cfg) {
    memcpy(cfg, &s_config, sizeof(argos_experiment_config_t));
    strcpy(cfg->nombre, "Onda Senoidal");
    strcpy(cfg->descripcion, "Genera una onda senoidal en el DAC");

    cfg->algoritmo.tipo = EXP_ALG_SENO;
    cfg->algoritmo.param1 = 1650.0f; /* Amplitud (mV) */
    cfg->algoritmo.param2 = 1650.0f; /* Offset (mV) */
    cfg->algoritmo.param3 = 1.0f;    /* Frecuencia (Hz) */
    cfg->intervalo_muestreo_ms = 20;
    cfg->numero_muestras = 500;

    cfg->num_columnas = 4;
    cfg->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    cfg->columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "lectura_mV", true};
    cfg->columnas[2] = (exp_column_config_t){EXP_COL_DAC_OUT, "seno_mV", true};
    cfg->columnas[3] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
}

static void plantilla_cuadrada(argos_experiment_config_t *cfg) {
    memcpy(cfg, &s_config, sizeof(argos_experiment_config_t));
    strcpy(cfg->nombre, "Onda Cuadrada");
    strcpy(cfg->descripcion, "Genera una onda cuadrada en el DAC");

    cfg->algoritmo.tipo = EXP_ALG_CUADRADA;
    cfg->algoritmo.param1 = 3300.0f; /* Alto (mV) */
    cfg->algoritmo.param2 = 0.0f;    /* Bajo (mV) */
    cfg->algoritmo.param3 = 500.0f;  /* Periodo (ms) */
    cfg->algoritmo.param4 = 50.0f;   /* Ciclo de trabajo (%) */
    cfg->intervalo_muestreo_ms = 10;
    cfg->numero_muestras = 1000;

    cfg->num_columnas = 4;
    cfg->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    cfg->columnas[1] = (exp_column_config_t){EXP_COL_VALOR_MV, "lectura_mV", true};
    cfg->columnas[2] = (exp_column_config_t){EXP_COL_DAC_OUT, "cuadrada_mV", true};
    cfg->columnas[3] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
}

/* ==================== FUNCIONES DE CONFIGURACIÓN ==================== */

void exp_config_init_default(argos_experiment_config_t *config) {
    if (config == NULL) return;

    memset(config, 0, sizeof(argos_experiment_config_t));

    strcpy(config->nombre, "Experimento sin nombre");
    strcpy(config->descripcion, "Configuración por defecto");
    config->version_config = 1;

    /* Canales ADC por defecto: 4 canales */
    config->num_canales_adc = 4;
    adc_channel_t adcs[] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5};
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

    /* Canales DAC por defecto: 2 canales */
    config->num_canales_dac = 2;
    dac_channel_t dacs[] = {DAC_CHAN_0, DAC_CHAN_1};
    const char *nombres_dac[] = {"DAC0-GPIO25", "DAC1-GPIO26"};
    for (int i = 0; i < 2; i++) {
        config->canales_dac[i].indice = i;
        config->canales_dac[i].tipo = EXP_SIGNAL_TIPO_DAC;
        config->canales_dac[i].dac_ch = dacs[i];
        config->canales_dac[i].habilitado = false;
        strcpy(config->canales_dac[i].nombre, nombres_dac[i]);
        strcpy(config->canales_dac[i].unidad, "mV");
    }

    /* PWM por defecto */
    config->num_canales_pwm = 2;
    config->canales_pwm[0].indice = 0;
    config->canales_pwm[0].tipo = EXP_SIGNAL_TIPO_PWM;
    config->canales_pwm[0].pwm_ch = 0;
    config->canales_pwm[0].habilitado = false;
    strcpy(config->canales_pwm[0].nombre, "PWM0-GPIO18");
    strcpy(config->canales_pwm[0].unidad, "%");
    config->canales_pwm[1].indice = 1;
    config->canales_pwm[1].tipo = EXP_SIGNAL_TIPO_PWM;
    config->canales_pwm[1].pwm_ch = 1;
    config->canales_pwm[1].habilitado = false;
    strcpy(config->canales_pwm[1].nombre, "PWM1-GPIO19");
    strcpy(config->canales_pwm[1].unidad, "%");

    /* Sensibilidad */
    config->atenuacion_adc = ADC_ATTEN_DB_11;
    config->resolucion_adc = ADC_WIDTH_BIT_12;

    /* Muestreo */
    config->intervalo_muestreo_ms = 100;
    config->numero_muestras = 0; /* Continuo */
    config->duracion_segundos = 0;

    /* Disparo */
    config->modo_disparo = EXP_TRIGGER_INMEDIATO;
    config->disparo_retardo_ms = 0;

    /* Columnas por defecto */
    config->num_columnas = 4;
    config->columnas[0] = (exp_column_config_t){EXP_COL_TIMESTAMP, "tiempo_ms", true};
    config->columnas[1] = (exp_column_config_t){EXP_COL_CHANNEL, "canal", true};
    config->columnas[2] = (exp_column_config_t){EXP_COL_VALOR_RAW, "valor_crudo", true};
    config->columnas[3] = (exp_column_config_t){EXP_COL_VALOR_MV, "voltaje_mV", true};

    /* Algoritmo */
    config->algoritmo.tipo = EXP_ALG_NINGUNO;
    strcpy(config->algoritmo.descripcion, "Captura directa sin procesamiento");

    /* Salida */
    config->incluir_encabezado_csv = true;
    config->usar_formato_json = false;
    config->decimales = 2;

    /* Pines de control */
    config->pin_trigger_in = -1;
    config->pin_trigger_out = -1;
    config->pin_led_estado = GPIO_NUM_2;
}

esp_err_t exp_config_apply_template(argos_experiment_config_t *config, const char *template) {
    if (config == NULL || template == NULL) return ESP_ERR_INVALID_ARG;

    /* Primero cargar valores por defecto */
    exp_config_init_default(config);

    if (strcmp(template, "barrido_dac") == 0) {
        plantilla_barrido_dac(config);
    } else if (strcmp(template, "lazo_cerrado_pid") == 0) {
        plantilla_lazo_cerrado_pid(config);
    } else if (strcmp(template, "rampa") == 0) {
        plantilla_rampa(config);
    } else if (strcmp(template, "seno") == 0) {
        plantilla_seno(config);
    } else if (strcmp(template, "cuadrada") == 0) {
        plantilla_cuadrada(config);
    } else if (strcmp(template, "default") == 0) {
        /* Ya está inicializado como default */
    } else {
        RTR_ERR("Plantilla desconocida: %s", template);
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

esp_err_t exp_config_save_template(const argos_experiment_config_t *config, const char *nombre) {
    if (config == NULL || nombre == NULL) return ESP_ERR_INVALID_ARG;

    /* Crear directorio de plantillas si no existe */
    struct stat st;
    if (stat(RUTA_TEMPLATES, &st) != 0) {
        mkdir(RUTA_TEMPLATES, 0755);
    }

    char ruta[128];
    snprintf(ruta, sizeof(ruta), "%s/%s%s", RUTA_TEMPLATES, nombre, EXP_FILE_EXT);

    /* Serializar a JSON manualmente */
    char buffer[2048];
    int n = exp_config_to_json(config, buffer, sizeof(buffer));
    if (n < 0) return ESP_FAIL;

    FILE *f = fopen(ruta, "w");
    if (f == NULL) {
        RTR_ERR("Error al guardar plantilla: %s", ruta);
        return ESP_FAIL;
    }
    fwrite(buffer, 1, n, f);
    fclose(f);

    RTR_LOG("Plantilla guardada: %s", ruta);
    return ESP_OK;
}

esp_err_t exp_config_load_template(argos_experiment_config_t *config, const char *nombre) {
    if (config == NULL || nombre == NULL) return ESP_ERR_INVALID_ARG;

    char ruta[128];
    snprintf(ruta, sizeof(ruta), "%s/%s%s", RUTA_TEMPLATES, nombre, EXP_FILE_EXT);

    FILE *f = fopen(ruta, "r");
    if (f == NULL) {
        /* Intentar cargar plantilla predefinida */
        return exp_config_apply_template(config, nombre);
    }

    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (tam <= 0 || tam > 4096) {
        fclose(f);
        return ESP_FAIL;
    }

    char *buffer = malloc(tam + 1);
    if (buffer == NULL) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    fread(buffer, 1, tam, f);
    buffer[tam] = '\0';
    fclose(f);

    esp_err_t ret = exp_config_from_json(config, buffer);
    free(buffer);
    return ret;
}

esp_err_t exp_config_list_templates(char templates[][EXP_CONFIG_TEMPLATE_NAME_MAX], size_t max_count, size_t *count) {
    if (templates == NULL || count == NULL) return ESP_ERR_INVALID_ARG;

    *count = 0;

    /* Plantillas predefinidas */
    const char *builtin[] = {"default", "barrido_dac", "lazo_cerrado_pid", "rampa", "seno", "cuadrada"};
    size_t n_builtin = sizeof(builtin) / sizeof(builtin[0]);

    for (size_t i = 0; i < n_builtin && *count < max_count; i++) {
        strncpy(templates[*count], builtin[i], EXP_CONFIG_TEMPLATE_NAME_MAX - 1);
        (*count)++;
    }

    /* Plantillas guardadas en SPIFFS */
    DIR *dir = opendir(RUTA_TEMPLATES);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && *count < max_count) {
            if (entry->d_type == DT_REG) {
                char *dot = strrchr(entry->d_name, '.');
                if (dot && strcmp(dot, EXP_FILE_EXT) == 0) {
                    *dot = '\0';
                    /* Evitar duplicados con las predefinidas */
                    bool duplicado = false;
                    for (size_t i = 0; i < *count; i++) {
                        if (strcmp(templates[i], entry->d_name) == 0) {
                            duplicado = true;
                            break;
                        }
                    }
                    if (!duplicado) {
                        strncpy(templates[*count], entry->d_name, EXP_CONFIG_TEMPLATE_NAME_MAX - 1);
                        (*count)++;
                    }
                }
            }
        }
        closedir(dir);
    }

    return ESP_OK;
}

int exp_config_generate_csv_header(const argos_experiment_config_t *config, char *buffer, size_t buffer_size) {
    if (config == NULL || buffer == NULL || buffer_size == 0) return 0;

    int n = 0;
    bool primera = true;

    for (uint8_t i = 0; i < config->num_columnas; i++) {
        if (!config->columnas[i].habilitada) continue;
        if (!primera) {
            n += snprintf(buffer + n, buffer_size - n, ",");
        }
        n += snprintf(buffer + n, buffer_size - n, "%s", config->columnas[i].encabezado);
        primera = false;
    }
    n += snprintf(buffer + n, buffer_size - n, "\n");

    return n;
}

int exp_config_measurement_to_csv(const argos_experiment_config_t *config,
                                   const argos_measurement_t *medicion,
                                   char *buffer, size_t buffer_size) {
    if (config == NULL || medicion == NULL || buffer == NULL || buffer_size == 0) return 0;

    int n = 0;
    bool primera = true;

    for (uint8_t i = 0; i < config->num_columnas; i++) {
        if (!config->columnas[i].habilitada) continue;

        if (!primera) {
            n += snprintf(buffer + n, buffer_size - n, ",");
        }

        switch (config->columnas[i].tipo) {
            case EXP_COL_TIMESTAMP:
                n += snprintf(buffer + n, buffer_size - n, "%lu", medicion->timestamp);
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
            case EXP_COL_VALOR_ESCALADO:
                n += snprintf(buffer + n, buffer_size - n, "%.*f", config->decimales,
                              medicion->value * config->canales_adc[medicion->channel].escala
                              + config->canales_adc[medicion->channel].offset);
                break;
            case EXP_COL_DAC_OUT:
                n += snprintf(buffer + n, buffer_size - n, "0");
                break;
            case EXP_COL_PWM_DUTY:
                n += snprintf(buffer + n, buffer_size - n, "0");
                break;
            default:
                n += snprintf(buffer + n, buffer_size - n, "0");
                break;
        }
        primera = false;
    }
    n += snprintf(buffer + n, buffer_size - n, "\n");

    return n;
}

int exp_config_to_json(const argos_experiment_config_t *config, char *buffer, size_t buffer_size) {
    if (config == NULL || buffer == NULL || buffer_size == 0) return -1;

    int n = snprintf(buffer, buffer_size,
        "{\n"
        "  \"nombre\": \"%s\",\n"
        "  \"descripcion\": \"%s\",\n"
        "  \"version\": %lu,\n"
        "  \"intervalo_muestreo_ms\": %lu,\n"
        "  \"numero_muestras\": %lu,\n"
        "  \"duracion_segundos\": %lu,\n"
        "  \"num_canales_adc\": %d,\n"
        "  \"num_canales_dac\": %d,\n"
        "  \"algoritmo\": %d\n"
        "}\n",
        config->nombre, config->descripcion,
        (unsigned long)config->version_config,
        (unsigned long)config->intervalo_muestreo_ms,
        (unsigned long)config->numero_muestras,
        (unsigned long)config->duracion_segundos,
        config->num_canales_adc, config->num_canales_dac,
        config->algoritmo.tipo);

    return (n > 0 && (size_t)n < buffer_size) ? n : -1;
}

esp_err_t exp_config_from_json(argos_experiment_config_t *config, const char *json) {
    if (config == NULL || json == NULL) return ESP_ERR_INVALID_ARG;

    exp_config_init_default(config);

    char *p = strstr(json, "\"nombre\"");
    if (p) {
        p = strchr(p, ':');
        if (p) {
            p = strchr(p, '"');
            if (p) {
                p++;
                char *end = strchr(p, '"');
                if (end) {
                    size_t len = (end - p < EXP_CONFIG_NAME_MAX - 1) ? (end - p) : (EXP_CONFIG_NAME_MAX - 1);
                    strncpy(config->nombre, p, len);
                    config->nombre[len] = '\0';
                }
            }
        }
    }

    p = strstr(json, "\"descripcion\"");
    if (p) {
        p = strchr(p, ':');
        if (p) {
            p = strchr(p, '"');
            if (p) {
                p++;
                char *end = strchr(p, '"');
                if (end) {
                    size_t len = (end - p < EXP_CONFIG_DESC_MAX - 1) ? (end - p) : (EXP_CONFIG_DESC_MAX - 1);
                    strncpy(config->descripcion, p, len);
                    config->descripcion[len] = '\0';
                }
            }
        }
    }

    p = strstr(json, "\"intervalo_muestreo_ms\"");
    if (p) {
        p = strchr(p, ':');
        if (p) config->intervalo_muestreo_ms = strtoul(p + 1, NULL, 10);
    }

    p = strstr(json, "\"numero_muestras\"");
    if (p) {
        p = strchr(p, ':');
        if (p) config->numero_muestras = strtoul(p + 1, NULL, 10);
    }

    p = strstr(json, "\"algoritmo\"");
    if (p) {
        p = strchr(p, ':');
        if (p) config->algoritmo.tipo = (exp_algorithm_type_t)atoi(p + 1);
    }

    return ESP_OK;
}

/* ==================== ALGORITMOS ==================== */

static esp_err_t algoritmo_barrido_dac(float entrada, float *salida) {
    float v_max = s_config.algoritmo.param1;
    float pasos = s_config.algoritmo.param2;
    if (pasos <= 0) pasos = 256;

    float v_salida = (s_algo_paso / pasos) * v_max;
    if (s_algo_paso >= (int)pasos) {
        s_algo_terminado = true;
        *salida = 0;
        return ESP_FAIL;
    }

    *salida = v_salida;
    s_algo_paso++;
    return ESP_OK;
}

static esp_err_t algoritmo_pid(float entrada, float *salida) {
    static float integral = 0.0f;
    static float error_anterior = 0.0f;

    float kp = s_config.algoritmo.param1;
    float ki = s_config.algoritmo.param2;
    float kd = s_config.algoritmo.param3;
    float setpoint = s_config.algoritmo.param4;

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

static esp_err_t algoritmo_rampa(float entrada, float *salida) {
    float v_max = s_config.algoritmo.param1;
    float duracion = s_config.algoritmo.param2;
    if (duracion <= 0) duracion = 500;

    float progreso = (float)(s_algo_paso * s_config.intervalo_muestreo_ms) / duracion;
    if (progreso >= 1.0f) {
        s_algo_paso = 0;
        progreso = 0.0f;
    }

    *salida = progreso * v_max;
    s_algo_paso++;
    return ESP_OK;
}

static esp_err_t algoritmo_seno(float entrada, float *salida) {
    float amplitud = s_config.algoritmo.param1;
    float offset = s_config.algoritmo.param2;
    float freq = s_config.algoritmo.param3;
    if (freq <= 0) freq = 1.0f;

    float t = s_algo_paso * s_config.intervalo_muestreo_ms / 1000.0f;
    *salida = offset + amplitud * sinf(2.0f * M_PI * freq * t);

    s_algo_paso++;
    return ESP_OK;
}

static esp_err_t algoritmo_cuadrada(float entrada, float *salida) {
    float alto = s_config.algoritmo.param1;
    float bajo = s_config.algoritmo.param2;
    float periodo_ms = s_config.algoritmo.param3;
    float duty = s_config.algoritmo.param4;
    if (periodo_ms <= 0) periodo_ms = 500;

    float t = s_algo_paso * s_config.intervalo_muestreo_ms;
    float ciclo = fmodf(t, periodo_ms);
    float umbral = periodo_ms * (duty / 100.0f);

    *salida = (ciclo < umbral) ? alto : bajo;
    s_algo_paso++;
    return ESP_OK;
}

/* ==================== IMPLEMENTACIÓN DEL ENRUTADOR ==================== */

esp_err_t argos_router_init(const argos_experiment_config_t *config) {
    if (s_inicializado) {
        RTR_WARN("Enrutador ya inicializado");
        return ESP_OK;
    }

    RTR_LOG("Inicializando enrutador...");

    if (config != NULL) {
        memcpy(&s_config, config, sizeof(argos_experiment_config_t));
    } else {
        exp_config_init_default(&s_config);
    }

    s_data_queue = xQueueCreate(ARGOS_ROUTER_QUEUE_LENGTH, sizeof(argos_measurement_t));
    if (s_data_queue == NULL) {
        RTR_ERR("Error al crear cola de datos");
        return ESP_ERR_NO_MEM;
    }

    s_control_queue = xQueueCreate(8, sizeof(uint32_t));
    if (s_control_queue == NULL) {
        vQueueDelete(s_data_queue);
        return ESP_ERR_NO_MEM;
    }

    s_config_mutex = xSemaphoreCreateMutex();
    s_state_mutex = xSemaphoreCreateMutex();

    if (s_config_mutex == NULL || s_state_mutex == NULL) {
        if (s_config_mutex) vSemaphoreDelete(s_config_mutex);
        if (s_state_mutex) vSemaphoreDelete(s_state_mutex);
        vQueueDelete(s_data_queue);
        vQueueDelete(s_control_queue);
        return ESP_ERR_NO_MEM;
    }

    s_estado = EXP_STATE_DETENIDO;
    s_total_mediciones = 0;
    s_errores = 0;
    s_inicializado = true;

    RTR_LOG("Enrutador inicializado");
    return ESP_OK;
}

esp_err_t argos_router_deinit(void) {
    if (!s_inicializado) return ESP_OK;

    if (s_estado == EXP_STATE_CORRIENDO) {
        argos_router_stop_experiment();
    }

    if (s_adq_task != NULL) {
        vTaskDelete(s_adq_task);
        s_adq_task = NULL;
    }

    vQueueDelete(s_data_queue);
    vQueueDelete(s_control_queue);
    vSemaphoreDelete(s_config_mutex);
    vSemaphoreDelete(s_state_mutex);

    s_inicializado = false;
    RTR_LOG("Enrutador desinicializado");
    return ESP_OK;
}

bool argos_router_is_ready(void) {
    return s_inicializado;
}

/* ==================== CONTROL DE EXPERIMENTO ==================== */

esp_err_t argos_router_start_experiment(void) {
    if (!s_inicializado) return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(s_state_mutex, portMAX_DELAY) == pdTRUE) {
        if (s_estado == EXP_STATE_CORRIENDO) {
            xSemaphoreGive(s_state_mutex);
            RTR_WARN("Experimento ya en ejecución");
            return ESP_OK;
        }
        s_estado = EXP_STATE_CORRIENDO;
        s_tiempo_inicio = esp_timer_get_time() / 1000000;
        s_algo_paso = 0;
        s_algo_terminado = false;
        s_total_mediciones = 0;
        xSemaphoreGive(s_state_mutex);
    }

    /* Aplicar retardo de disparo */
    if (s_config.disparo_retardo_ms > 0) {
        RTR_LOG("Retardo de disparo: %lu ms", s_config.disparo_retardo_ms);
        vTaskDelay(pdMS_TO_TICKS(s_config.disparo_retardo_ms));
    }

    /* Crear archivo de log con la configuración actual */
    argos_store_create_log_file(s_config.usar_formato_json ? ARGOS_STORE_FORMAT_JSON : ARGOS_STORE_FORMAT_CSV);

    RTR_LOG("Experimento iniciado: %s", s_config.nombre);
    return ESP_OK;
}

esp_err_t argos_router_stop_experiment(void) {
    if (!s_inicializado) return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(s_state_mutex, portMAX_DELAY) == pdTRUE) {
        if (s_estado != EXP_STATE_CORRIENDO) {
            xSemaphoreGive(s_state_mutex);
            return ESP_OK;
        }
        s_estado = EXP_STATE_DETENIDO;
        xSemaphoreGive(s_state_mutex);
    }

    argos_store_flush();
    argos_store_close_log_file();

    RTR_LOG("Experimento detenido. Mediciones: %lu", s_total_mediciones);
    return ESP_OK;
}

esp_err_t argos_router_pause_experiment(bool pausar) {
    if (!s_inicializado) return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(s_state_mutex, portMAX_DELAY) == pdTRUE) {
        s_estado = pausar ? EXP_STATE_PAUSADO : EXP_STATE_CORRIENDO;
        xSemaphoreGive(s_state_mutex);
    }

    RTR_LOG("Experimento %s", pausar ? "pausado" : "reanudado");
    return ESP_OK;
}

exp_state_t argos_router_get_state(void) {
    exp_state_t estado;
    if (xSemaphoreTake(s_state_mutex, portMAX_DELAY) == pdTRUE) {
        estado = s_estado;
        xSemaphoreGive(s_state_mutex);
    } else {
        estado = EXP_STATE_ERROR;
    }
    return estado;
}

/* ==================== GESTIÓN DE CONFIGURACIÓN ==================== */

const argos_experiment_config_t *argos_router_get_config(void) {
    return &s_config;
}

esp_err_t argos_router_set_config(const argos_experiment_config_t *config) {
    if (config == NULL) return ESP_ERR_INVALID_ARG;
    if (s_estado != EXP_STATE_DETENIDO) {
        RTR_ERR("No se puede cambiar configuración durante ejecución");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_config_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&s_config, config, sizeof(argos_experiment_config_t));
        xSemaphoreGive(s_config_mutex);
    }
    return ESP_OK;
}

esp_err_t argos_router_load_template(const char *nombre) {
    if (nombre == NULL) return ESP_ERR_INVALID_ARG;

    argos_experiment_config_t nueva_config;
    esp_err_t ret = exp_config_load_template(&nueva_config, nombre);
    if (ret != ESP_OK) return ret;

    return argos_router_set_config(&nueva_config);
}

esp_err_t argos_router_save_template(const char *nombre) {
    if (nombre == NULL) return ESP_ERR_INVALID_ARG;
    return exp_config_save_template(&s_config, nombre);
}

/* ==================== ENRUTAMIENTO ==================== */

int argos_router_route_measurement(const argos_measurement_t *medicion) {
    if (!s_inicializado || medicion == NULL) return 0;

    int entregados = 0;

    /* Enviar a todos los destinos */
    if (argos_router_route_to(medicion, ARGOS_ROUTER_DEST_SERIAL) == ESP_OK) entregados++;
    if (argos_router_route_to(medicion, ARGOS_ROUTER_DEST_STORE) == ESP_OK) entregados++;
    if (argos_router_route_to(medicion, ARGOS_ROUTER_DEST_WEBSOCKET) == ESP_OK) entregados++;

    s_total_mediciones++;

    return entregados;
}

esp_err_t argos_router_route_to(const argos_measurement_t *medicion, argos_router_dest_t destino) {
    if (!s_inicializado || medicion == NULL) return ESP_ERR_INVALID_ARG;

    switch (destino) {
        case ARGOS_ROUTER_DEST_SERIAL:
            ESP_LOGI("DATA", "%lu,%d,%.2f", medicion->timestamp, medicion->channel, medicion->value);
            break;

        case ARGOS_ROUTER_DEST_STORE: {
            /* Formatear según configuración de columnas y guardar */
            char linea[256];
            exp_config_measurement_to_csv(&s_config, medicion, linea, sizeof(linea));
            argos_store_write_measurement(medicion);
            break;
        }

        case ARGOS_ROUTER_DEST_WEBSOCKET:
            argos_net_ws_send_measurement(medicion);
            break;

        default:
            return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}

/* ==================== ALGORITMOS ==================== */

esp_err_t argos_router_run_algorithm_step(float entrada, float *salida) {
    if (salida == NULL) return ESP_ERR_INVALID_ARG;

    if (s_algo_terminado) {
        return ESP_FAIL;
    }

    switch (s_config.algoritmo.tipo) {
        case EXP_ALG_NINGUNO:
            *salida = 0;
            return ESP_OK;
        case EXP_ALG_BARRIDO_DAC:
            return algoritmo_barrido_dac(entrada, salida);
        case EXP_ALG_LAZO_CERRADO_PID:
            return algoritmo_pid(entrada, salida);
        case EXP_ALG_RAMPA:
            return algoritmo_rampa(entrada, salida);
        case EXP_ALG_SENO:
            return algoritmo_seno(entrada, salida);
        case EXP_ALG_CUADRADA:
            return algoritmo_cuadrada(entrada, salida);
        default:
            *salida = 0;
            return ESP_OK;
    }
}

QueueHandle_t argos_router_get_data_queue(void) {
    return s_data_queue;
}

QueueHandle_t argos_router_get_control_queue(void) {
    return s_control_queue;
}

/* ==================== TAREA DE ADQUISICIÓN ==================== */

esp_err_t argos_router_start_acquisition_task(void) {
    if (!s_inicializado) return ESP_ERR_INVALID_STATE;

    if (s_adq_task != NULL) {
        RTR_WARN("Tarea de adquisición ya creada");
        return ESP_OK;
    }

    BaseType_t creada = xTaskCreatePinnedToCore(
        argos_router_acquisition_task,
        "router_adq",
        4096,
        NULL,
        6,
        &s_adq_task,
        tskNO_AFFINITY
    );

    if (creada != pdPASS) {
        RTR_ERR("Error al crear tarea de adquisición");
        return ESP_ERR_NO_MEM;
    }

    RTR_LOG("Tarea de adquisición iniciada");
    return ESP_OK;
}

void argos_router_acquisition_task(void *arg) {
    adc_channel_t adc_channels[EXP_CONFIG_MAX_CHANNELS];
    int num_adc = 0;

    /* Obtener canales ADC habilitados */
    for (int i = 0; i < s_config.num_canales_adc; i++) {
        if (s_config.canales_adc[i].habilitado) {
            adc_channels[num_adc++] = s_config.canales_adc[i].adc_ch;
        }
    }

    uint32_t contador = 0;

    while (1) {
        exp_state_t estado;
        if (xSemaphoreTake(s_state_mutex, portMAX_DELAY) == pdTRUE) {
            estado = s_estado;
            xSemaphoreGive(s_state_mutex);
        } else {
            estado = EXP_STATE_DETENIDO;
        }

        if (estado == EXP_STATE_CORRIENDO) {
            uint32_t voltajes[EXP_CONFIG_MAX_CHANNELS];

            for (int i = 0; i < num_adc; i++) {
                uint32_t mv = 0;
                esp_err_t ret = argos_hal_adc_read_voltage(adc_channels[i], &mv);

                if (ret == ESP_OK) {
                    argos_measurement_t medicion = {
                        .timestamp = contador + i,
                        .channel = (uint8_t)i,
                        .value = mv / 1000.0f
                    };

                    /* Ejecutar algoritmo si está configurado */
                    float salida = 0;
                    esp_err_t algo_ret = argos_router_run_algorithm_step((float)mv, &salida);

                    if (s_config.num_canales_dac > 0 && s_config.canales_dac[0].habilitado) {
                        argos_hal_dac_write_voltage(DAC_CHAN_0, (uint32_t)salida);
                    }

                    /* Enrutar medición a todos los destinos */
                    argos_router_route_measurement(&medicion);
                    xQueueSend(s_data_queue, &medicion, 0);

                    if (algo_ret != ESP_OK) {
                        s_algo_terminado = false;
                        if (s_config.numero_muestras > 0) {
                            RTR_LOG("Algoritmo completado, deteniendo experimento");
                            argos_router_stop_experiment();
                        }
                    }
                } else {
                    s_errores++;
                }
            }
            contador += num_adc;

            /* Verificar límite de muestras */
            if (s_config.numero_muestras > 0 && contador >= s_config.numero_muestras) {
                RTR_LOG("Número de muestras alcanzado: %lu", s_config.numero_muestras);
                argos_router_stop_experiment();
            }

            /* Verificar límite de tiempo */
            if (s_config.duracion_segundos > 0) {
                int64_t ahora = esp_timer_get_time() / 1000000;
                if (ahora - s_tiempo_inicio >= s_config.duracion_segundos) {
                    RTR_LOG("Duración alcanzada: %lu s", s_config.duracion_segundos);
                    argos_router_stop_experiment();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(s_config.intervalo_muestreo_ms));
    }
}

/* ==================== LOGGING MULTICANAL ==================== */

void argos_router_log(int level, const char *tag, const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    switch (level) {
        case ESP_LOG_INFO:
            ESP_LOGI(tag, "%s", buffer);
            break;
        case ESP_LOG_WARN:
            ESP_LOGW(tag, "%s", buffer);
            break;
        case ESP_LOG_ERROR:
            ESP_LOGE(tag, "%s", buffer);
            break;
        default:
            ESP_LOGI(tag, "%s", buffer);
            break;
    }
}

/* ==================== ESTADÍSTICAS Y DEBUG ==================== */

esp_err_t argos_router_get_stats(argos_router_stats_t *stats) {
    if (stats == NULL) return ESP_ERR_INVALID_ARG;

    stats->total_mediciones = s_total_mediciones;
    stats->errores = s_errores;

    if (s_tiempo_inicio > 0) {
        stats->tiempo_activo_seg = (uint32_t)((esp_timer_get_time() / 1000000) - s_tiempo_inicio);
    } else {
        stats->tiempo_activo_seg = 0;
    }

    if (stats->tiempo_activo_seg > 0) {
        stats->mediciones_segundo = s_total_mediciones / stats->tiempo_activo_seg;
    } else {
        stats->mediciones_segundo = 0;
    }

    stats->uso_buffer_pct = 0;

    return ESP_OK;
}

void argos_router_print_diagnostics(void) {
    RTR_LOG("========== DIAGNÓSTICO DEL ENRUTADOR ==========");
    RTR_LOG("Estado: %s (inicializado=%s)",
            s_estado == EXP_STATE_DETENIDO ? "DETENIDO" :
            s_estado == EXP_STATE_CORRIENDO ? "CORRIENDO" :
            s_estado == EXP_STATE_PAUSADO ? "PAUSADO" :
            s_estado == EXP_STATE_COMPLETADO ? "COMPLETADO" : "ERROR",
            s_inicializado ? "SÍ" : "NO");

    RTR_LOG("Experimento: %s", s_config.nombre);
    RTR_LOG("Algoritmo: %d", s_config.algoritmo.tipo);
    RTR_LOG("Intervalo: %lu ms, Muestras: %lu",
            s_config.intervalo_muestreo_ms, s_config.numero_muestras);
    RTR_LOG("Canales ADC: %d, DAC: %d",
            s_config.num_canales_adc, s_config.num_canales_dac);
    RTR_LOG("Columnas en CSV: %d", s_config.num_columnas);

    argos_router_stats_t stats;
    if (argos_router_get_stats(&stats) == ESP_OK) {
        RTR_LOG("Total mediciones: %lu, Errores: %lu", stats.total_mediciones, stats.errores);
        RTR_LOG("Tiempo activo: %lu s, Velocidad: %lu med/s",
                stats.tiempo_activo_seg, stats.mediciones_segundo);
    }

    RTR_LOG("==============================================");
}

esp_err_t argos_router_self_test(void) {
    RTR_LOG("Iniciando auto-test del enrutador...");

    if (!s_inicializado) {
        RTR_ERR("Auto-test falló: enrutador no inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    RTR_LOG("Auto-test: estado=%d, cola datos=%s, cola control=%s",
            s_estado,
            s_data_queue ? "OK" : "NULA",
            s_control_queue ? "OK" : "NULA");

    /* Probar enrutamiento de una medición de prueba */
    argos_measurement_t prueba = {0, 0, 1.5f};
    int entregados = argos_router_route_measurement(&prueba);
    RTR_LOG("Auto-test: ruteo=%d destinos", entregados);

    /* Probar algoritmos */
    float salida = 0;
    esp_err_t algo = argos_router_run_algorithm_step(1000.0f, &salida);
    RTR_LOG("Auto-test: algoritmo=%s, salida=%.2f",
            algo == ESP_OK ? "OK" : "TERMINADO", salida);

    /* Probar configuración de plantillas */
    argos_experiment_config_t cfg_test;
    exp_config_init_default(&cfg_test);
    RTR_LOG("Auto-test: config por defecto OK");

    /* Probar encabezado CSV */
    char csv_header[256];
    int n = exp_config_generate_csv_header(&s_config, csv_header, sizeof(csv_header));
    if (n > 0) {
        csv_header[n] = '\0';
        RTR_LOG("Auto-test: CSV header=%s", csv_header);
    }

    RTR_LOG("Auto-test del enrutador COMPLETADO");
    return ESP_OK;
}
