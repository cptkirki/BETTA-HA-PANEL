#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "app_config.h"

typedef struct {
    const char *ssid;
    const char *password;
    uint8_t channel;
    uint8_t max_connection;
} wifi_mgr_ap_config_t;

typedef struct {
    char ssid[APP_WIFI_SSID_MAX_LEN];
    int8_t rssi;
    uint8_t authmode;
} wifi_mgr_scan_result_t;

typedef struct {
    const char *ssid;
    const char *password;
    bool wait_for_ip;
    int connect_timeout_ms;
    int max_retries;
} wifi_mgr_config_t;

esp_err_t wifi_mgr_init(const wifi_mgr_config_t *cfg);
bool wifi_mgr_is_connected(void);
esp_err_t wifi_mgr_force_reconnect(void);
esp_err_t wifi_mgr_start_setup_ap(const wifi_mgr_ap_config_t *cfg);
esp_err_t wifi_mgr_stop_setup_ap(void);
bool wifi_mgr_is_setup_ap_active(void);
const char *wifi_mgr_get_setup_ap_ssid(void);
esp_err_t wifi_mgr_get_sta_ip(char *out, size_t out_len);
esp_err_t wifi_mgr_get_ap_ip(char *out, size_t out_len);
esp_err_t wifi_mgr_scan(wifi_mgr_scan_result_t *results, size_t max_results, size_t *out_count);
