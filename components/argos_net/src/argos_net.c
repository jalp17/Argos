#include "argos_net.h"
#include "argos_store.h"
#include "net_config.h"
#include "webpage.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = ARGOS_NET_DEBUG_TAG;

/* ==================== MACROS DE DEPURACIÓN ==================== */
#if ARGOS_NET_DEBUG_ENABLE
#define NET_LOG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define NET_ERR(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#define NET_WARN(fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
#else
#define NET_LOG(fmt, ...)
#define NET_ERR(fmt, ...)
#define NET_WARN(fmt, ...)
#endif

/* ==================== ESTADO INTERNO ==================== */
static bool s_inicializado = false;
static bool s_ap_iniciado = false;
static bool s_servidor_iniciado = false;
static esp_netif_t *s_netif_ap = NULL;
static httpd_handle_t s_servidor = NULL;
static SemaphoreHandle_t s_ws_mutex = NULL;

#define ARGOS_NET_MAX_WS_CLIENTS 8
static int s_ws_client_fds[ARGOS_NET_MAX_WS_CLIENTS];
static int s_ws_client_count = 0;

static char s_ap_ssid[32] = ARGOS_NET_AP_SSID;
static char s_ap_ip[16] = ARGOS_NET_AP_IP;

/* ==================== FUNCIONES AUXILIARES ==================== */

static void agregar_cliente_ws(int fd) {
    if (xSemaphoreTake(s_ws_mutex, portMAX_DELAY) == pdTRUE) {
        if (s_ws_client_count < ARGOS_NET_MAX_WS_CLIENTS) {
            s_ws_client_fds[s_ws_client_count++] = fd;
            NET_LOG("Cliente WS conectado: fd=%d, total=%d", fd, s_ws_client_count);
        }
        xSemaphoreGive(s_ws_mutex);
    }
}

static void eliminar_cliente_ws(int fd) {
    if (xSemaphoreTake(s_ws_mutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < s_ws_client_count; i++) {
            if (s_ws_client_fds[i] == fd) {
                for (int j = i; j < s_ws_client_count - 1; j++) {
                    s_ws_client_fds[j] = s_ws_client_fds[j + 1];
                }
                s_ws_client_count--;
                NET_LOG("Cliente WS desconectado: fd=%d, total=%d", fd, s_ws_client_count);
                break;
            }
        }
        xSemaphoreGive(s_ws_mutex);
    }
}

/* ==================== MANEJADORES HTTP ==================== */

static esp_err_t manejador_raiz(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_send(req, ARGOS_WEBPAGE_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t manejador_api_estado(httpd_req_t *req) {
    char buffer[512];
    argos_store_stats_t stats;
    argos_store_get_stats(&stats);

    int n = snprintf(buffer, sizeof(buffer),
        "{\"tipo\":\"estado\",\"ssid\":\"%s\",\"ip\":\"%s\",\"clientes\":%d,\"almacenamiento\":\"%d%% usado\",\"uptime\":\"%llds\"}",
        s_ap_ssid, s_ap_ip, s_ws_client_count, stats.usage_percent,
        (long long)(esp_timer_get_time() / 1000000));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, buffer, n);
    return ESP_OK;
}

static esp_err_t manejador_api_archivos(httpd_req_t *req) {
    argos_store_file_info_t archivos[20];
    size_t count = 0;
    esp_err_t ret = argos_store_list_files(archivos, 20, &count);
    if (ret != ESP_OK) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, "{\"error\":\"Error al listar archivos\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    size_t tam = 256 + count * 128;
    char *buffer = calloc(1, tam);
    if (buffer == NULL) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, "{\"error\":\"Sin memoria\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_ERR_NO_MEM;
    }

    int n = snprintf(buffer, tam, "{\"tipo\":\"archivos\",\"archivos\":[");
    for (size_t i = 0; i < count; i++) {
        n += snprintf(buffer + n, tam - n,
            "%s{\"nombre\":\"%s\",\"tamano\":%d}",
            (i > 0) ? "," : "", archivos[i].filename, archivos[i].size);
    }
    n += snprintf(buffer + n, tam - n, "]}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, buffer, n);
    free(buffer);
    return ESP_OK;
}

static esp_err_t manejador_api_descargar(httpd_req_t *req) {
    char archivo[64] = {0};
    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len > 0 && query_len < 128) {
        char query[128];
        httpd_req_get_url_query_str(req, query, sizeof(query));
        char param[32];
        if (httpd_query_key_value(query, "archivo", param, sizeof(param)) == ESP_OK) {
            strncpy(archivo, param, sizeof(archivo) - 1);
        }
    }

    if (archivo[0] == '\0') {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Falta parametro 'archivo'", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    uint8_t *buffer = malloc(16384);
    if (buffer == NULL) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, "Sin memoria", HTTPD_RESP_USE_STRLEN);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_leidos = 0;
    esp_err_t ret = argos_store_export_file(archivo, buffer, 16384, &bytes_leidos);
    if (ret != ESP_OK) {
        free(buffer);
        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_send(req, "Archivo no encontrado", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/csv; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment");
    httpd_resp_send(req, (char *)buffer, bytes_leidos);
    free(buffer);
    return ESP_OK;
}

/* ==================== MANEJADOR WEBSOCKET ==================== */

static esp_err_t manejador_websocket(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        NET_LOG("Handshake WS iniciado");
        return ESP_OK;
    }

    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, ARGOS_NET_WS_FRAME_MAX);
    if (ret != ESP_OK || frame.len == 0) {
        return ret;
    }

    uint8_t *buf = calloc(1, frame.len + 1);
    if (buf == NULL) return ESP_ERR_NO_MEM;

    frame.payload = buf;
    ret = httpd_ws_recv_frame(req, &frame, frame.len);
    if (ret != ESP_OK) {
        free(buf);
        return ret;
    }

    /* Procesar mensaje de control desde el cliente */
    buf[frame.len] = '\0';
    NET_LOG("Mensaje WS recibido: %s", buf);
    free(buf);
    return ESP_OK;
}

static esp_err_t manejador_ws_open(httpd_handle_t hd, int fd) {
    agregar_cliente_ws(fd);
    return ESP_OK;
}

static void manejador_ws_close(httpd_handle_t hd, int fd) {
    eliminar_cliente_ws(fd);
}

/* ==================== REGISTRO DE MANEJADORES ==================== */

static esp_err_t registrar_manejadores(httpd_handle_t servidor) {
    httpd_uri_t uris[] = {
        {.uri = "/", .method = HTTP_GET, .handler = manejador_raiz, .user_ctx = NULL},
        {.uri = "/api/estado", .method = HTTP_GET, .handler = manejador_api_estado, .user_ctx = NULL},
        {.uri = "/api/archivos", .method = HTTP_GET, .handler = manejador_api_archivos, .user_ctx = NULL},
        {.uri = "/api/descargar", .method = HTTP_GET, .handler = manejador_api_descargar, .user_ctx = NULL},
    };

    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        esp_err_t ret = httpd_register_uri_handler(servidor, &uris[i]);
        if (ret != ESP_OK) {
            NET_ERR("Error al registrar URI %s: %s", uris[i].uri, esp_err_to_name(ret));
            return ret;
        }
        NET_LOG("URI registrada: %s", uris[i].uri);
    }

    /* Configurar WebSocket */
    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = manejador_websocket,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true
    };
    esp_err_t ret = httpd_register_uri_handler(servidor, &ws);
    if (ret == ESP_OK) {
        NET_LOG("WebSocket registrado en /ws");
    }

    return ESP_OK;
}

/* ==================== API PÚBLICA ==================== */

esp_err_t argos_net_ap_init(void) {
    if (s_ap_iniciado) {
        NET_WARN("AP ya iniciado");
        return ESP_OK;
    }

    NET_LOG("Iniciando SoftAP...");

    esp_netif_init();
    esp_event_loop_create_default();

    s_netif_ap = esp_netif_create_default_wifi_ap();
    if (s_netif_ap == NULL) {
        NET_ERR("Error al crear interfaz de red AP");
        return ESP_FAIL;
    }

    /* Configurar IP estática */
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(s_netif_ap);
    esp_netif_set_ip_info(s_netif_ap, &ip_info);
    esp_netif_dhcps_start(s_netif_ap);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = ARGOS_NET_AP_SSID,
            .password = ARGOS_NET_AP_PASSWORD,
            .ssid_len = strlen(ARGOS_NET_AP_SSID),
            .channel = ARGOS_NET_AP_CHANNEL,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = ARGOS_NET_AP_MAX_CONN,
        },
    };

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (ret != ESP_OK) {
        NET_ERR("Error al configurar AP: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        NET_ERR("Error al iniciar WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    s_ap_iniciado = true;
    NET_LOG("SoftAP iniciado: SSID=%s, IP=%s", ARGOS_NET_AP_SSID, ARGOS_NET_AP_IP);
    return ESP_OK;
}

esp_err_t argos_net_server_init(void) {
    if (s_servidor_iniciado) {
        NET_WARN("Servidor ya iniciado");
        return ESP_OK;
    }

    NET_LOG("Iniciando servidor web...");

    s_ws_mutex = xSemaphoreCreateMutex();
    if (s_ws_mutex == NULL) {
        NET_ERR("Error al crear mutex para WS");
        return ESP_ERR_NO_MEM;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = ARGOS_NET_HTTP_PORT;
    config.stack_size = ARGOS_NET_HTTP_STACK_SIZE;
    config.max_uri_handlers = 12;
    config.lru_purge_enable = true;
    config.close_fn = manejador_ws_close;
    config.open_fn = manejador_ws_open;

    esp_err_t ret = httpd_start(&s_servidor, &config);
    if (ret != ESP_OK) {
        NET_ERR("Error al iniciar servidor HTTP: %s", esp_err_to_name(ret));
        vSemaphoreDelete(s_ws_mutex);
        return ret;
    }

    ret = registrar_manejadores(s_servidor);
    if (ret != ESP_OK) {
        httpd_stop(s_servidor);
        vSemaphoreDelete(s_ws_mutex);
        return ret;
    }

    s_servidor_iniciado = true;
    NET_LOG("Servidor web iniciado en puerto %d", ARGOS_NET_HTTP_PORT);
    return ESP_OK;
}

esp_err_t argos_net_init(void) {
    if (s_inicializado) {
        NET_WARN("Red ya inicializada");
        return ESP_OK;
    }

    NET_LOG("Inicializando componente de red...");

    esp_err_t ret = argos_net_ap_init();
    if (ret != ESP_OK) return ret;

    ret = argos_net_server_init();
    if (ret != ESP_OK) {
        esp_wifi_stop();
        s_ap_iniciado = false;
        return ret;
    }

    s_inicializado = true;
    NET_LOG("Componente de red inicializado correctamente");
    return ESP_OK;
}

esp_err_t argos_net_deinit(void) {
    if (!s_inicializado) return ESP_OK;

    if (s_servidor_iniciado) {
        httpd_stop(s_servidor);
        s_servidor = NULL;
        s_servidor_iniciado = false;
    }

    if (s_ws_mutex != NULL) {
        vSemaphoreDelete(s_ws_mutex);
        s_ws_mutex = NULL;
    }

    if (s_ap_iniciado) {
        esp_wifi_stop();
        esp_wifi_deinit();
        s_ap_iniciado = false;
    }

    s_ws_client_count = 0;
    s_inicializado = false;
    NET_LOG("Componente de red desinicializado");
    return ESP_OK;
}

bool argos_net_is_ready(void) {
    return s_inicializado && s_ap_iniciado && s_servidor_iniciado;
}

int argos_net_ws_broadcast(const char *data, size_t len) {
    if (!s_servidor_iniciado || s_ws_mutex == NULL) return 0;

    int enviados = 0;
    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = (uint8_t *)data;
    frame.len = len;

    if (xSemaphoreTake(s_ws_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < s_ws_client_count; i++) {
            int fd = s_ws_client_fds[i];
            esp_err_t ret = httpd_ws_send_frame_async(s_servidor, fd, &frame);
            if (ret == ESP_OK) {
                enviados++;
            } else {
                NET_ERR("Error al enviar WS a fd=%d: %s", fd, esp_err_to_name(ret));
            }
        }
        xSemaphoreGive(s_ws_mutex);
    }

    return enviados;
}

int argos_net_ws_send_measurement(const argos_measurement_t *medicion) {
    if (medicion == NULL) return 0;

    char buffer[128];
    int n = snprintf(buffer, sizeof(buffer),
        "{\"tipo\":\"medicion\",\"canales\":[%d,%d,%d,%d]}",
        (int)(medicion->value * 1000), 0, 0, 0);

    return argos_net_ws_broadcast(buffer, n);
}

int argos_net_ws_get_client_count(void) {
    return s_ws_client_count;
}

httpd_handle_t argos_net_get_server(void) {
    return s_servidor;
}

void argos_net_get_ap_ip(char *ip) {
    if (ip != NULL) strcpy(ip, s_ap_ip);
}

void argos_net_get_ap_ssid(char *ssid) {
    if (ssid != NULL) strcpy(ssid, s_ap_ssid);
}

void argos_net_print_diagnostics(void) {
    NET_LOG("========== DIAGNÓSTICO DE RED ==========");
    NET_LOG("Inicializado: %s", s_inicializado ? "SÍ" : "NO");
    NET_LOG("AP: %s (%s)", s_ap_ssid, s_ap_ip);
    NET_LOG("Servidor HTTP: puerto %d", ARGOS_NET_HTTP_PORT);
    NET_LOG("Clientes WS conectados: %d", s_ws_client_count);
    NET_LOG("========================================");
}

esp_err_t argos_net_self_test(void) {
    NET_LOG("Iniciando auto-test de red...");

    if (!s_inicializado) {
        NET_ERR("Auto-test falló: red no inicializada");
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_ap_iniciado) {
        NET_ERR("Auto-test falló: AP no iniciado");
        return ESP_ERR_INVALID_STATE;
    }
    NET_LOG("Auto-test: AP iniciado OK (%s)", s_ap_ssid);

    if (!s_servidor_iniciado) {
        NET_ERR("Auto-test falló: servidor no iniciado");
        return ESP_ERR_INVALID_STATE;
    }
    NET_LOG("Auto-test: servidor HTTP OK");

    NET_LOG("Auto-test de red COMPLETADO");
    return ESP_OK;
}
