/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "app_config.h"

typedef struct {
    const char *ssid;
    const char *password;
    const char *country_code;
    uint8_t channel;
    uint8_t max_connection;
} wifi_mgr_ap_config_t;

typedef struct {
    char ssid[APP_WIFI_SSID_MAX_LEN];
    int8_t rssi;
    uint8_t authmode;
    uint8_t channel;
    uint8_t bssid[6];
    bool connected;
} wifi_mgr_scan_result_t;

typedef struct {
    const char *ssid;
    const char *password;
    const char *country_code;
    const char *bssid;
    bool wait_for_ip;
    int connect_timeout_ms;
    int max_retries;
} wifi_mgr_config_t;

typedef struct {
    char ssid[APP_WIFI_SSID_MAX_LEN];
    int8_t rssi;
    uint8_t authmode;
    uint8_t channel;
    uint8_t bssid[6];
} wifi_mgr_sta_ap_info_t;

esp_err_t wifi_mgr_init(const wifi_mgr_config_t *cfg);
bool wifi_mgr_is_connected(void);
esp_err_t wifi_mgr_force_reconnect(void);
esp_err_t wifi_mgr_force_transport_recover(void);
esp_err_t wifi_mgr_start_setup_ap(const wifi_mgr_ap_config_t *cfg);
esp_err_t wifi_mgr_stop_setup_ap(void);
bool wifi_mgr_is_setup_ap_active(void);
const char *wifi_mgr_get_setup_ap_ssid(void);
esp_err_t wifi_mgr_get_sta_ip(char *out, size_t out_len);
esp_err_t wifi_mgr_get_ap_ip(char *out, size_t out_len);
esp_err_t wifi_mgr_get_sta_ap_info(wifi_mgr_sta_ap_info_t *out_info);
esp_err_t wifi_mgr_get_sta_rssi(int8_t *out_rssi_dbm);
esp_err_t wifi_mgr_scan(wifi_mgr_scan_result_t *results, size_t max_results, size_t *out_count);
