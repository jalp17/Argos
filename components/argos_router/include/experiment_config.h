#ifndef EXPERIMENT_CONFIG_H
#define EXPERIMENT_CONFIG_H

#include "argos_core.h"
#include "driver/adc.h"
#include "driver/dac.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuración completa de un experimento para Argos.
 *
 * Define qué pines usar, sensibilidad, intervalo de muestreo,
 * orden de columnas y modos de disparo.
 */

/* ==================== CONSTANTES ==================== */
#define EXP_CONFIG_MAX_CHANNELS      8
#define EXP_CONFIG_MAX_COLUMNS       16
#define EXP_CONFIG_NAME_MAX          64
#define EXP_CONFIG_DESC_MAX          256
#define EXP_CONFIG_TEMPLATE_MAX      10
#define EXP_CONFIG_TEMPLATE_NAME_MAX 32

/* ==================== MODOS DE DISPARO ==================== */
typedef enum {
    EXP_TRIGGER_INMEDIATO = 0,   /* Inicia al presionar "Iniciar" */
    EXP_TRIGGER_MANUAL = 1,      /* Inicia con botón físico */
    EXP_TRIGGER_EXTERNO = 2,     /* Inicia con señal externa */
    EXP_TRIGGER_PROGRAMADO = 3   /* Inicia a una hora específica */
} exp_trigger_mode_t;

/* ==================== TIPO DE SEÑAL ==================== */
typedef enum {
    EXP_SIGNAL_TIPO_ADC = 0,     /* Entrada analógica */
    EXP_SIGNAL_TIPO_DAC = 1,     /* Salida analógica */
    EXP_SIGNAL_TIPO_PWM = 2,     /* Salida PWM */
    EXP_SIGNAL_TIPO_GPIO = 3     /* Entrada/Salida digital */
} exp_signal_type_t;

/* ==================== CONFIGURACIÓN DE CANAL ==================== */
typedef struct {
    uint8_t indice;                  /* Índice del canal (0-7) */
    exp_signal_type_t tipo;          /* ADC, DAC, PWM o GPIO */
    union {
        adc_channel_t adc_ch;        /* Canal ADC si tipo=ADC */
        dac_channel_t dac_ch;        /* Canal DAC si tipo=DAC */
        uint8_t pwm_ch;              /* Canal PWM si tipo=PWM */
        uint8_t gpio_num;            /* GPIO si tipo=GPIO */
    };
    bool habilitado;                 /* true si el canal está activo */
    char nombre[24];                 /* Nombre descriptivo (ej: "Sensor Temp") */
    char unidad[12];                 /* Unidad de medida (ej: "mV", "°C") */
    float escala;                    /* Factor de escala */
    float offset;                    /* Offset de calibración */
} exp_channel_config_t;

/* ==================== CONFIGURACIÓN DE COLUMNAS ==================== */
typedef enum {
    EXP_COL_TIMESTAMP = 0,         /* Marca de tiempo */
    EXP_COL_CHANNEL,               /* Número de canal */
    EXP_COL_VALOR_RAW,             /* Valor crudo ADC */
    EXP_COL_VALOR_MV,              /* Valor en milivoltios */
    EXP_COL_VALOR_ESCALADO,        /* Valor con escala aplicada */
    EXP_COL_DAC_OUT,               /* Valor de salida DAC */
    EXP_COL_PWM_DUTY               /* Ciclo de trabajo PWM */
} exp_column_type_t;

typedef struct {
    exp_column_type_t tipo;        /* Tipo de columna */
    char encabezado[24];           /* Encabezado en el CSV (ej: "tiempo_ms") */
    bool habilitada;               /* true si se incluye en la salida */
} exp_column_config_t;

/* ==================== ALGORITMO DE PRÁCTICA ==================== */
typedef enum {
    EXP_ALG_NINGUNO = 0,          /* Solo captura directa */
    EXP_ALG_BARRIDO_DAC,          /* Barrido automático DAC mientras lee ADC */
    EXP_ALG_LAZO_CERRADO_PID,     /* Control PID en lazo cerrado */
    EXP_ALG_RAMPA,                /* Generación de rampa en DAC */
    EXP_ALG_SENO,                 /* Generación de onda senoidal en DAC */
    EXP_ALG_CUADRADA,             /* Generación de onda cuadrada en DAC */
    EXP_ALG_PERSONALIZADO         /* Algoritmo definido por el usuario */
} exp_algorithm_type_t;

typedef struct {
    exp_algorithm_type_t tipo;     /* Tipo de algoritmo */
    float param1;                  /* Parámetro genérico 1 */
    float param2;                  /* Parámetro genérico 2 */
    float param3;                  /* Parámetro genérico 3 */
    float param4;                  /* Parámetro genérico 4 */
    char descripcion[128];         /* Descripción del algoritmo */
} exp_algorithm_config_t;

/* ==================== CONFIGURACIÓN PRINCIPAL ==================== */
typedef struct {
    /* Metadatos del experimento */
    char nombre[EXP_CONFIG_NAME_MAX];
    char descripcion[EXP_CONFIG_DESC_MAX];
    uint32_t version_config;

    /* Configuración de pines y sensibilidad */
    exp_channel_config_t canales_adc[EXP_CONFIG_MAX_CHANNELS];
    exp_channel_config_t canales_dac[EXP_CONFIG_MAX_CHANNELS];
    exp_channel_config_t canales_pwm[EXP_CONFIG_MAX_CHANNELS];
    uint8_t num_canales_adc;
    uint8_t num_canales_dac;
    uint8_t num_canales_pwm;

    /* Sensibilidad ADC */
    adc_atten_t atenuacion_adc;    /* ADC_ATTEN_DB_0, _2_5, _6, _11 */
    adc_bits_width_t resolucion_adc; /* ADC_WIDTH_BIT_9, _10, _11, _12 */

    /* Intervalo y muestras */
    uint32_t intervalo_muestreo_ms;  /* Intervalo entre muestras en ms */
    uint32_t numero_muestras;        /* 0 = continuo, N = número fijo */
    uint32_t duracion_segundos;      /* 0 = sin límite de tiempo */

    /* Modo de disparo */
    exp_trigger_mode_t modo_disparo;
    uint32_t disparo_retardo_ms;     /* Retardo antes de iniciar */

    /* Orden de columnas en el archivo generado */
    exp_column_config_t columnas[EXP_CONFIG_MAX_COLUMNS];
    uint8_t num_columnas;

    /* Algoritmo de práctica */
    exp_algorithm_config_t algoritmo;

    /* Configuración de salida */
    bool incluir_encabezado_csv;     /* Incluir fila de encabezado */
    bool usar_formato_json;          /* true=JSON, false=CSV */
    uint8_t decimales;               /* Número de decimales (2-6) */

    /* Pines de control */
    int pin_trigger_in;
    int pin_trigger_out;
    int pin_led_estado;

} argos_experiment_config_t;

/* ==================== ESTADO DEL EXPERIMENTO ==================== */
typedef enum {
    EXP_STATE_DETENIDO = 0,
    EXP_STATE_CORRIENDO = 1,
    EXP_STATE_PAUSADO = 2,
    EXP_STATE_COMPLETADO = 3,
    EXP_STATE_ERROR = 4
} exp_state_t;

/* ==================== PLANTILLAS ==================== */
typedef struct {
    char nombre[EXP_CONFIG_TEMPLATE_NAME_MAX];
    char descripcion[128];
    argos_experiment_config_t config;
} exp_template_t;

/* ==================== CONFIGURACIÓN POR DEFECTO ==================== */

/**
 * @brief Inicializa una configuración de experimento con valores por defecto.
 * @param config Puntero a la configuración a inicializar.
 */
void exp_config_init_default(argos_experiment_config_t *config);

/**
 * @brief Aplica una plantilla predefinida a la configuración.
 * @param config Puntero a la configuración.
 * @param template Nombre de la plantilla ("barrido_dac", "pid", "rampa", etc.)
 * @return ESP_OK si se aplicó correctamente.
 */
esp_err_t exp_config_apply_template(argos_experiment_config_t *config, const char *template);

/**
 * @brief Guarda la configuración actual como plantilla en SPIFFS.
 * @param config Puntero a la configuración.
 * @param nombre Nombre de la plantilla.
 * @return ESP_OK en éxito.
 */
esp_err_t exp_config_save_template(const argos_experiment_config_t *config, const char *nombre);

/**
 * @brief Carga una plantilla desde SPIFFS.
 * @param config Puntero donde cargar la configuración.
 * @param nombre Nombre de la plantilla a cargar.
 * @return ESP_OK en éxito.
 */
esp_err_t exp_config_load_template(argos_experiment_config_t *config, const char *nombre);

/**
 * @brief Lista las plantillas guardadas.
 * @param templates Array de nombres de plantillas.
 * @param max_count Máximo de plantillas a listar.
 * @param count Puntero para el número real de plantillas.
 * @return ESP_OK en éxito.
 */
esp_err_t exp_config_list_templates(char templates[][EXP_CONFIG_TEMPLATE_NAME_MAX], size_t max_count, size_t *count);

/**
 * @brief Genera el encabezado CSV según la configuración de columnas.
 * @param config Puntero a la configuración.
 * @param buffer Buffer para el encabezado.
 * @param buffer_size Tamaño del buffer.
 * @return Número de bytes escritos.
 */
int exp_config_generate_csv_header(const argos_experiment_config_t *config, char *buffer, size_t buffer_size);

/**
 * @brief Serializa una medición a CSV según el orden de columnas configurado.
 * @param config Puntero a la configuración.
 * @param medicion Puntero a la medición.
 * @param buffer Buffer para la línea CSV.
 * @param buffer_size Tamaño del buffer.
 * @return Número de bytes escritos.
 */
int exp_config_measurement_to_csv(const argos_experiment_config_t *config,
                                   const argos_measurement_t *medicion,
                                   char *buffer, size_t buffer_size);

/**
 * @brief Convierte la configuración a JSON.
 * @param config Puntero a la configuración.
 * @param buffer Buffer de salida.
 * @param buffer_size Tamaño del buffer.
 * @return Número de bytes escritos o código de error negativo.
 */
int exp_config_to_json(const argos_experiment_config_t *config, char *buffer, size_t buffer_size);

/**
 * @brief Carga configuración desde JSON.
 * @param config Puntero donde cargar.
 * @param json Cadena JSON.
 * @return ESP_OK en éxito.
 */
esp_err_t exp_config_from_json(argos_experiment_config_t *config, const char *json);

#ifdef __cplusplus
}
#endif

#endif // EXPERIMENT_CONFIG_H
