#ifndef NET_CONFIG_H
#define NET_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuración de red para Argos.
 *
 * Define SSID, contraseña, IP estática y puertos del servidor web.
 * Toda la configuración puede modificarse via menuconfig o directamente aquí.
 */

/* ==================== CONFIGURACIÓN SOFTAP ==================== */
#define ARGOS_NET_AP_SSID          "Argos-AP"
#define ARGOS_NET_AP_PASSWORD      "argos1234"
#define ARGOS_NET_AP_MAX_CONN      4
#define ARGOS_NET_AP_CHANNEL       6

/* ==================== CONFIGURACIÓN IP ESTÁTICA ==================== */
#define ARGOS_NET_AP_IP            "192.168.4.1"
#define ARGOS_NET_AP_GATEWAY       "192.168.4.1"
#define ARGOS_NET_AP_NETMASK       "255.255.255.0"

/* ==================== CONFIGURACIÓN SERVIDOR ==================== */
#define ARGOS_NET_HTTP_PORT        80
#define ARGOS_NET_WS_PORT          81
#define ARGOS_NET_HTTP_STACK_SIZE  4096
#define ARGOS_NET_WS_STACK_SIZE    4096

/* ==================== LÍMITES ==================== */
#define ARGOS_NET_MAX_URI_LEN      64
#define ARGOS_NET_MAX_QUERY_LEN    128
#define ARGOS_NET_MAX_POST_DATA    1024
#define ARGOS_NET_WS_FRAME_MAX     512

/* ==================== DEBUG ==================== */
#define ARGOS_NET_DEBUG_ENABLE     1
#define ARGOS_NET_DEBUG_TAG        "argos_net"

/* ==================== TIMEOUTS ==================== */
#define ARGOS_NET_WS_PING_INTERVAL 30   // segundos
#define ARGOS_NET_WS_BUFFER_SIZE   1024

#ifdef __cplusplus
}
#endif

#endif // NET_CONFIG_H
