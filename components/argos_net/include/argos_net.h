#ifndef ARGOS_NET_H
#define ARGOS_NET_H

#include "argos_core.h"
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Componente de red para Argos.
 *
 * Proporciona SoftAP, servidor web HTTP, WebSockets en tiempo real
 * y API REST para control y descarga de datos.
 */

/* ==================== TIPOS ==================== */

typedef enum {
    ARGOS_NET_EVENT_CLIENT_CONNECTED = 0,
    ARGOS_NET_EVENT_CLIENT_DISCONNECTED = 1,
    ARGOS_NET_EVENT_DATA_SENT = 2,
    ARGOS_NET_EVENT_ERROR = 3
} argos_net_event_t;

typedef struct {
    uint32_t ip_address;
    uint16_t port;
    bool is_websocket;
    uint32_t connected_since;
} argos_net_client_t;

/* ==================== INICIALIZACIÓN ==================== */

/**
 * @brief Inicializa el SoftAP con IP estática.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_net_ap_init(void);

/**
 * @brief Inicializa el servidor web HTTP y WebSockets.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_net_server_init(void);

/**
 * @brief Inicializa todo el componente de red (AP + servidor).
 * @return ESP_OK en éxito.
 */
esp_err_t argos_net_init(void);

/**
 * @brief Detiene el servidor y cierra el AP.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_net_deinit(void);

/**
 * @brief Verifica si el componente de red está listo.
 * @return true si está listo.
 */
bool argos_net_is_ready(void);

/* ==================== WEBSOCKETS ==================== */

/**
 * @brief Envía datos JSON a todos los clientes WebSocket conectados.
 * @param data Cadena JSON a enviar.
 * @param len Longitud de los datos.
 * @return Número de clientes a los que se envió.
 */
int argos_net_ws_broadcast(const char *data, size_t len);

/**
 * @brief Envía una medición formateada como JSON a todos los clientes WS.
 * @param measurement Puntero a la medición.
 * @return Número de clientes a los que se envió.
 */
int argos_net_ws_send_measurement(const argos_measurement_t *measurement);

/**
 * @brief Obtiene el número de clientes WebSocket conectados.
 * @return Número de clientes.
 */
int argos_net_ws_get_client_count(void);

/* ==================== API REST ==================== */

/**
 * @brief Obtiene el manejador del servidor HTTP (para agregar endpoints personalizados).
 * @return Manejador del servidor HTTP.
 */
httpd_handle_t argos_net_get_server(void);

/* ==================== ESTADÍSTICAS ==================== */

/**
 * @brief Obtiene la IP del AP.
 * @param ip Buffer para la IP (mínimo 16 bytes).
 */
void argos_net_get_ap_ip(char *ip);

/**
 * @brief Obtiene el SSID del AP.
 * @param ssid Buffer para el SSID (mínimo 32 bytes).
 */
void argos_net_get_ap_ssid(char *ssid);

/* ==================== DEBUG ==================== */

/**
 * @brief Imprime diagnóstico de red.
 */
void argos_net_print_diagnostics(void);

/**
 * @brief Realiza auto-test del componente de red.
 * @return ESP_OK en éxito.
 */
esp_err_t argos_net_self_test(void);

#ifdef __cplusplus
}
#endif

#endif // ARGOS_NET_H
