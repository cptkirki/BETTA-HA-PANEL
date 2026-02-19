/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    HA_WS_EVENT_CONNECTED = 0,
    HA_WS_EVENT_DISCONNECTED,
    HA_WS_EVENT_TEXT,
    HA_WS_EVENT_ERROR,
} ha_ws_event_type_t;

typedef struct {
    ha_ws_event_type_t type;
    const char *data;
    int data_len;
    bool fin;
    uint8_t op_code;
    int payload_len;
    int payload_offset;
    esp_err_t tls_esp_err;
    int tls_stack_err;
    int tls_cert_flags;
    int ws_handshake_status_code;
    int sock_errno;
} ha_ws_event_t;

typedef void (*ha_ws_event_cb_t)(const ha_ws_event_t *event, void *user_ctx);

typedef struct {
    const char *uri;
    ha_ws_event_cb_t event_cb;
    void *user_ctx;
} ha_ws_config_t;

esp_err_t ha_ws_start(const ha_ws_config_t *cfg);
void ha_ws_stop(void);
bool ha_ws_is_connected(void);
bool ha_ws_is_running(void);
esp_err_t ha_ws_send_text(const char *text);
bool ha_ws_get_cached_resolved_ipv4(char *host_out, size_t host_out_sz, char *ip_out, size_t ip_out_sz);
