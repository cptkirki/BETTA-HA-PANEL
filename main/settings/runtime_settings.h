#pragma once

#include <stdbool.h>

#include "esp_err.h"

#include "app_config.h"

typedef struct {
    char wifi_ssid[APP_WIFI_SSID_MAX_LEN];
    char wifi_password[APP_WIFI_PASSWORD_MAX_LEN];
    char wifi_country_code[APP_WIFI_COUNTRY_CODE_MAX_LEN];
    char wifi_bssid[APP_WIFI_BSSID_MAX_LEN];
    char ha_ws_url[APP_HA_WS_URL_MAX_LEN];
    char ha_access_token[APP_HA_ACCESS_TOKEN_MAX_LEN];
    bool ha_rest_enabled;
    char ntp_server[APP_NTP_SERVER_MAX_LEN];
    char time_tz[APP_TIME_TZ_MAX_LEN];
    char ui_language[APP_UI_LANGUAGE_MAX_LEN];
} runtime_settings_t;

void runtime_settings_set_defaults(runtime_settings_t *out);
esp_err_t runtime_settings_init(void);
esp_err_t runtime_settings_load(runtime_settings_t *out);
esp_err_t runtime_settings_save(const runtime_settings_t *settings);
bool runtime_settings_has_wifi(const runtime_settings_t *settings);
bool runtime_settings_has_ha(const runtime_settings_t *settings);
