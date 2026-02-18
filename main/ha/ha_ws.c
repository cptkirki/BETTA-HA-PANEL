#include "ha/ha_ws.h"

#include <stdlib.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"

#include "util/log_tags.h"
#include "esp_websocket_client.h"
#define HA_WS_HAS_ESP_WS_CLIENT 1


#if HA_WS_HAS_ESP_WS_CLIENT
static esp_websocket_client_handle_t s_ws_client = NULL;
#else
static void *s_ws_client = NULL;
#endif
static ha_ws_config_t s_cfg = {0};
static volatile bool s_connected = false;
static char *s_uri_owned = NULL;
static char s_uri_runtime[320] = {0};
static char s_tls_common_name[128] = {0};
static char s_ws_headers[192] = {0};
static char s_last_resolved_host[128] = {0};
static char s_last_resolved_ip[64] = {0};

#define HA_WS_TASK_STACK 12288
#define HA_WS_BUFFER_SIZE 16384
#define HA_WS_CTRL_PING_INTERVAL_SEC 25
#define HA_WS_CTRL_PINGPONG_TIMEOUT_SEC 15
#define HA_WS_TCP_KEEPALIVE_IDLE_SEC 30
#define HA_WS_TCP_KEEPALIVE_INTERVAL_SEC 10
#define HA_WS_TCP_KEEPALIVE_COUNT 3
#define HA_WS_NETWORK_TIMEOUT_MS 30000
#define HA_WS_RECONNECT_TIMEOUT_MS 5000

static bool parse_ws_uri(const char *uri, bool *is_secure, char *host, size_t host_sz, int *port, const char **path_out)
{
    if (uri == NULL || is_secure == NULL || host == NULL || host_sz == 0 || port == NULL || path_out == NULL) {
        return false;
    }

    const char *ws = "ws://";
    const char *wss = "wss://";
    const char *p = NULL;

    if (strncmp(uri, wss, strlen(wss)) == 0) {
        *is_secure = true;
        *port = 443;
        p = uri + strlen(wss);
    } else if (strncmp(uri, ws, strlen(ws)) == 0) {
        *is_secure = false;
        *port = 80;
        p = uri + strlen(ws);
    } else {
        return false;
    }

    const char *path = strchr(p, '/');
    size_t authority_len = 0;
    if (path == NULL) {
        *path_out = "/";
        authority_len = strlen(p);
    } else {
        *path_out = path;
        authority_len = (size_t)(path - p);
    }
    if (authority_len == 0) {
        return false;
    }

    const char *colon = NULL;
    for (size_t i = 0; i < authority_len; i++) {
        if (p[i] == ':') {
            colon = &p[i];
        }
    }

    size_t host_len = authority_len;
    if (colon != NULL) {
        host_len = (size_t)(colon - p);
        const char *port_str = colon + 1;
        if (port_str < p + authority_len) {
            int parsed_port = atoi(port_str);
            if (parsed_port > 0 && parsed_port <= 65535) {
                *port = parsed_port;
            }
        }
    }

    if (host_len == 0 || host_len >= host_sz) {
        return false;
    }

    memcpy(host, p, host_len);
    host[host_len] = '\0';
    return true;
}

static bool resolve_host_ipv4(const char *host, char *ip_out, size_t ip_out_sz)
{
    if (host == NULL || ip_out == NULL || ip_out_sz == 0) {
        return false;
    }

    struct in_addr addr4 = {0};
    if (inet_pton(AF_INET, host, &addr4) == 1) {
        strlcpy(ip_out, host, ip_out_sz);
        return true;
    }

    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(host, NULL, &hints, &result);
    if (rc != 0 || result == NULL) {
        if (result != NULL) {
            freeaddrinfo(result);
        }
        return false;
    }

    bool ok = false;
    if (result->ai_family == AF_INET && result->ai_addrlen >= sizeof(struct sockaddr_in)) {
        struct sockaddr_in *sa = (struct sockaddr_in *)result->ai_addr;
        if (inet_ntop(AF_INET, &sa->sin_addr, ip_out, ip_out_sz) != NULL) {
            ok = true;
        }
    }
    freeaddrinfo(result);
    return ok;
}

static const char *build_runtime_uri(const char *uri)
{
    s_uri_runtime[0] = '\0';
    s_ws_headers[0] = '\0';
    s_tls_common_name[0] = '\0';

    bool secure = false;
    char host[128] = {0};
    int port = 0;
    const char *path = NULL;

    if (!parse_ws_uri(uri, &secure, host, sizeof(host), &port, &path)) {
        return uri;
    }

    strlcpy(s_tls_common_name, host, sizeof(s_tls_common_name));

    char ip[64] = {0};
    if (resolve_host_ipv4(host, ip, sizeof(ip))) {
        strlcpy(s_last_resolved_host, host, sizeof(s_last_resolved_host));
        strlcpy(s_last_resolved_ip, ip, sizeof(s_last_resolved_ip));
        return uri;
    }

    if (s_last_resolved_host[0] != '\0' && s_last_resolved_ip[0] != '\0' &&
        strcmp(s_last_resolved_host, host) == 0) {
        const char *scheme = secure ? "wss" : "ws";
        if (snprintf(s_uri_runtime, sizeof(s_uri_runtime), "%s://%s:%d%s", scheme, s_last_resolved_ip, port, path) <= 0) {
            s_uri_runtime[0] = '\0';
            return NULL;
        }
        snprintf(s_ws_headers, sizeof(s_ws_headers), "Host: %s:%d\r\n", host, port);
        ESP_LOGW(TAG_HA_WS, "DNS resolve failed for '%s', using cached IP %s", host, s_last_resolved_ip);
        return s_uri_runtime;
    }

    ESP_LOGW(TAG_HA_WS, "DNS resolve failed for '%s' and no cached IP available", host);
    return NULL;
}

static void ws_dispatch_event(
    ha_ws_event_type_t type, const char *data, int len, bool fin, uint8_t op_code, int payload_len, int payload_offset,
    esp_err_t tls_esp_err, int tls_stack_err, int tls_cert_flags, int ws_handshake_status_code, int sock_errno)
{
    if (s_cfg.event_cb == NULL) {
        return;
    }
    ha_ws_event_t event = {
        .type = type,
        .data = data,
        .data_len = len,
        .fin = fin,
        .op_code = op_code,
        .payload_len = payload_len,
        .payload_offset = payload_offset,
        .tls_esp_err = tls_esp_err,
        .tls_stack_err = tls_stack_err,
        .tls_cert_flags = tls_cert_flags,
        .ws_handshake_status_code = ws_handshake_status_code,
        .sock_errno = sock_errno,
    };
    s_cfg.event_cb(&event, s_cfg.user_ctx);
}

#if HA_WS_HAS_ESP_WS_CLIENT
static void ws_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_base;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG_HA_WS, "Connected");
        ws_dispatch_event(HA_WS_EVENT_CONNECTED, NULL, 0, true, 0, 0, 0, ESP_OK, 0, 0, 0, 0);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG_HA_WS, "Disconnected");
        ws_dispatch_event(HA_WS_EVENT_DISCONNECTED, NULL, 0, true, 0, 0, 0, ESP_OK, 0, 0, 0, 0);
        break;
    case WEBSOCKET_EVENT_DATA:
        if (data != NULL && data->op_code == WS_TRANSPORT_OPCODES_PING) {
            /* esp_websocket_client auto-replies to control PING frames.
               Avoid sending an additional manual PONG from callback context. */
            ESP_LOGD(TAG_HA_WS, "WS control PING received");
            break;
        }
        if (data != NULL &&
            (data->op_code == WS_TRANSPORT_OPCODES_TEXT || data->op_code == WS_TRANSPORT_OPCODES_CONT)) {
            ws_dispatch_event(HA_WS_EVENT_TEXT, (const char *)data->data_ptr, data->data_len, data->fin,
                data->op_code, data->payload_len, data->payload_offset, ESP_OK, 0, 0, 0, 0);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        esp_err_t tls_esp_err = ESP_OK;
        int tls_stack_err = 0;
        int tls_cert_flags = 0;
        int ws_handshake_status_code = 0;
        int sock_errno = 0;
        if (data != NULL) {
            tls_esp_err = data->error_handle.esp_tls_last_esp_err;
            tls_stack_err = data->error_handle.esp_tls_stack_err;
            tls_cert_flags = data->error_handle.esp_tls_cert_verify_flags;
            ws_handshake_status_code = data->error_handle.esp_ws_handshake_status_code;
            sock_errno = data->error_handle.esp_transport_sock_errno;
        }
        ESP_LOGE(TAG_HA_WS, "WebSocket error");
        ws_dispatch_event(HA_WS_EVENT_ERROR, NULL, 0, true, 0, 0, 0,
            tls_esp_err, tls_stack_err, tls_cert_flags, ws_handshake_status_code, sock_errno);
        break;
    default:
        break;
    }
}
#endif

esp_err_t ha_ws_start(const ha_ws_config_t *cfg)
{
    if (cfg == NULL || cfg->uri == NULL || cfg->uri[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_ws_client != NULL) {
        return ESP_OK;
    }

    char *uri_copy = strdup(cfg->uri);
    if (uri_copy == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (s_uri_owned != NULL) {
        free(s_uri_owned);
        s_uri_owned = NULL;
    }
    s_uri_owned = uri_copy;

    s_cfg = *cfg;

#if HA_WS_HAS_ESP_WS_CLIENT
    const char *runtime_uri = build_runtime_uri(s_uri_owned);
    if (runtime_uri == NULL) {
        ESP_LOGW(TAG_HA_WS, "Skipping WS start until HA host can be resolved again");
        return ESP_ERR_NOT_FOUND;
    }
    const bool is_secure_ws = (strncmp(runtime_uri, "wss://", 6) == 0);
    esp_websocket_client_config_t ws_cfg = {
        .uri = runtime_uri,
        .disable_auto_reconnect = true,
        .task_stack = HA_WS_TASK_STACK,
        .buffer_size = HA_WS_BUFFER_SIZE,
        .ping_interval_sec = HA_WS_CTRL_PING_INTERVAL_SEC,
        .pingpong_timeout_sec = HA_WS_CTRL_PINGPONG_TIMEOUT_SEC,
        .disable_pingpong_discon = true,
        .keep_alive_enable = true,
        .keep_alive_idle = HA_WS_TCP_KEEPALIVE_IDLE_SEC,
        .keep_alive_interval = HA_WS_TCP_KEEPALIVE_INTERVAL_SEC,
        .keep_alive_count = HA_WS_TCP_KEEPALIVE_COUNT,
        .network_timeout_ms = HA_WS_NETWORK_TIMEOUT_MS,
        .reconnect_timeout_ms = HA_WS_RECONNECT_TIMEOUT_MS,
    };
    if (s_ws_headers[0] != '\0') {
        ws_cfg.headers = s_ws_headers;
    }
    if (is_secure_ws) {
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
        ws_cfg.crt_bundle_attach = esp_crt_bundle_attach;
#else
        ESP_LOGW(TAG_HA_WS,
            "WSS requested but CONFIG_MBEDTLS_CERTIFICATE_BUNDLE is disabled; server verification may fail");
#endif
        if (s_tls_common_name[0] != '\0') {
            ws_cfg.cert_common_name = s_tls_common_name;
        }
    }

    s_ws_client = esp_websocket_client_init(&ws_cfg);
    if (s_ws_client == NULL) {
        free(s_uri_owned);
        s_uri_owned = NULL;
        return ESP_FAIL;
    }

    esp_err_t err = esp_websocket_register_events(s_ws_client, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL);
    if (err != ESP_OK) {
        esp_websocket_client_destroy(s_ws_client);
        s_ws_client = NULL;
        free(s_uri_owned);
        s_uri_owned = NULL;
        return err;
    }

    err = esp_websocket_client_start(s_ws_client);
    if (err != ESP_OK) {
        esp_websocket_client_destroy(s_ws_client);
        s_ws_client = NULL;
        free(s_uri_owned);
        s_uri_owned = NULL;
        return err;
    }
    return ESP_OK;
#else
    ESP_LOGW(TAG_HA_WS, "esp_websocket_client component not available, HA websocket disabled");
    s_connected = false;
    ws_dispatch_event(HA_WS_EVENT_ERROR, NULL, 0, true, 0, 0, 0, ESP_ERR_NOT_SUPPORTED, 0, 0, 0, 0);
    free(s_uri_owned);
    s_uri_owned = NULL;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

void ha_ws_stop(void)
{
    s_connected = false;
#if HA_WS_HAS_ESP_WS_CLIENT
    if (s_ws_client != NULL) {
        /* Avoid noisy "Client was not started" warnings after transport errors:
           stop only if the client still reports active connection. */
        if (esp_websocket_client_is_connected(s_ws_client)) {
            esp_websocket_client_stop(s_ws_client);
        }
        esp_websocket_client_destroy(s_ws_client);
        s_ws_client = NULL;
    }
#endif
    if (s_uri_owned != NULL) {
        free(s_uri_owned);
        s_uri_owned = NULL;
    }
}

bool ha_ws_is_connected(void)
{
    return s_connected;
}

bool ha_ws_is_running(void)
{
#if HA_WS_HAS_ESP_WS_CLIENT
    return s_ws_client != NULL;
#else
    return false;
#endif
}

bool ha_ws_get_cached_resolved_ipv4(char *host_out, size_t host_out_sz, char *ip_out, size_t ip_out_sz)
{
    if (host_out == NULL || host_out_sz == 0 || ip_out == NULL || ip_out_sz == 0) {
        return false;
    }
    if (s_last_resolved_host[0] == '\0' || s_last_resolved_ip[0] == '\0') {
        return false;
    }

    strlcpy(host_out, s_last_resolved_host, host_out_sz);
    strlcpy(ip_out, s_last_resolved_ip, ip_out_sz);
    return true;
}

esp_err_t ha_ws_send_text(const char *text)
{
#if HA_WS_HAS_ESP_WS_CLIENT
    if (text == NULL || s_ws_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!ha_ws_is_connected()) {
        return ESP_ERR_INVALID_STATE;
    }
    int written = esp_websocket_client_send_text(s_ws_client, text, strlen(text), pdMS_TO_TICKS(150));
    if (written > 0) {
        return ESP_OK;
    }

    /* Mark as disconnected on send failure so upper layers can recover. */
    s_connected = false;
    return ESP_FAIL;
#else
    (void)text;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
