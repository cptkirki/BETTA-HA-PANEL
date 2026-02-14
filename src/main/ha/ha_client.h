#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    const char *ws_url;
    const char *access_token;
} ha_client_config_t;

esp_err_t ha_client_start(const ha_client_config_t *cfg);
void ha_client_stop(void);
bool ha_client_is_connected(void);
bool ha_client_is_initial_sync_done(void);
esp_err_t ha_client_call_service(const char *domain, const char *service, const char *json_service_data);
esp_err_t ha_client_notify_layout_updated(void);
