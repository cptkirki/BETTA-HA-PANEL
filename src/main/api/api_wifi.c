#include "api/api_routes.h"

#include <stdbool.h>
#include <stdlib.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_wifi.h"

#include "net/wifi_mgr.h"

#define WIFI_SCAN_MAX_RESULTS 40

static void set_json_headers(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

static const char *authmode_to_string(uint8_t authmode)
{
    switch ((wifi_auth_mode_t)authmode) {
    case WIFI_AUTH_OPEN:
        return "open";
    case WIFI_AUTH_WEP:
        return "wep";
    case WIFI_AUTH_WPA_PSK:
        return "wpa_psk";
    case WIFI_AUTH_WPA2_PSK:
        return "wpa2_psk";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "wpa_wpa2_psk";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "wpa2_enterprise";
    case WIFI_AUTH_WPA3_PSK:
        return "wpa3_psk";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "wpa2_wpa3_psk";
#if defined(WIFI_AUTH_WPA3_ENTERPRISE)
    case WIFI_AUTH_WPA3_ENTERPRISE:
        return "wpa3_enterprise";
#endif
#if defined(WIFI_AUTH_WPA2_WPA3_ENTERPRISE)
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
        return "wpa2_wpa3_enterprise";
#endif
#if defined(WIFI_AUTH_WPA3_ENT_192)
    case WIFI_AUTH_WPA3_ENT_192:
        return "wpa3_enterprise_192";
#endif
#if defined(WIFI_AUTH_OWE)
    case WIFI_AUTH_OWE:
        return "owe";
#endif
#if defined(WIFI_AUTH_OWE_TRANSITION)
    case WIFI_AUTH_OWE_TRANSITION:
        return "owe_transition";
#endif
#if defined(WIFI_AUTH_WAPI_PSK)
    case WIFI_AUTH_WAPI_PSK:
        return "wapi_psk";
#endif
    default:
        return "unknown";
    }
}

static bool authmode_is_secure(uint8_t authmode)
{
    return (wifi_auth_mode_t)authmode != WIFI_AUTH_OPEN;
}

static const char *scan_error_http_status(esp_err_t scan_err)
{
    switch (scan_err) {
    case ESP_ERR_TIMEOUT:
        return "504 Gateway Timeout";
    case ESP_ERR_INVALID_STATE:
        return "503 Service Unavailable";
    case ESP_ERR_NO_MEM:
        return "503 Service Unavailable";
    case ESP_ERR_NOT_SUPPORTED:
        return "501 Not Implemented";
    default:
        return "500 Internal Server Error";
    }
}

static const char *scan_error_message(esp_err_t scan_err)
{
    switch (scan_err) {
    case ESP_ERR_TIMEOUT:
        return "Wi-Fi scan timed out";
    case ESP_ERR_INVALID_STATE:
        return "Wi-Fi scan currently unavailable";
    case ESP_ERR_NO_MEM:
        return "Device is temporarily busy";
    case ESP_ERR_NOT_SUPPORTED:
        return "Wi-Fi scan is unavailable in setup AP mode on this hardware";
    default:
        return "Wi-Fi scan failed";
    }
}

static esp_err_t send_scan_error(httpd_req_t *req, esp_err_t scan_err)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return httpd_resp_send_500(req);
    }
    cJSON_AddBoolToObject(root, "ok", false);
    cJSON_AddStringToObject(root, "error", esp_err_to_name(scan_err));
    cJSON_AddStringToObject(root, "message", scan_error_message(scan_err));
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }

    set_json_headers(req);
    httpd_resp_set_status(req, scan_error_http_status(scan_err));
    esp_err_t err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);
    return err;
}

esp_err_t api_wifi_scan_get_handler(httpd_req_t *req)
{
    wifi_mgr_scan_result_t results[WIFI_SCAN_MAX_RESULTS] = {0};
    size_t count = 0;
    esp_err_t scan_err = wifi_mgr_scan(results, WIFI_SCAN_MAX_RESULTS, &count);
    if (scan_err != ESP_OK) {
        return send_scan_error(req, scan_err);
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();
    if (root == NULL || items == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(items);
        return httpd_resp_send_500(req);
    }

    for (size_t i = 0; i < count; i++) {
        cJSON *entry = cJSON_CreateObject();
        if (entry == NULL) {
            cJSON_Delete(root);
            cJSON_Delete(items);
            return httpd_resp_send_500(req);
        }

        cJSON_AddStringToObject(entry, "ssid", results[i].ssid);
        cJSON_AddNumberToObject(entry, "rssi", (double)results[i].rssi);
        cJSON_AddStringToObject(entry, "authmode", authmode_to_string(results[i].authmode));
        cJSON_AddBoolToObject(entry, "secure", authmode_is_secure(results[i].authmode));
        cJSON_AddItemToArray(items, entry);
    }

    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddNumberToObject(root, "count", (double)count);
    cJSON_AddItemToObject(root, "items", items);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }

    set_json_headers(req);
    esp_err_t err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);
    return err;
}
