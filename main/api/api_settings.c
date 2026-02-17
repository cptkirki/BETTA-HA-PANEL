#include "api/api_routes.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "app_config.h"
#include "bsp/display.h"
#include "ha/ha_client.h"
#include "net/wifi_mgr.h"
#include "settings/i18n_store.h"
#include "settings/runtime_settings.h"

static esp_timer_handle_t s_restart_timer = NULL;

static void set_json_headers(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

static esp_err_t send_json_error(httpd_req_t *req, const char *status, const char *message)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return httpd_resp_send_500(req);
    }
    cJSON_AddBoolToObject(root, "ok", false);
    cJSON_AddStringToObject(root, "error", (message != NULL) ? message : "Invalid request");
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }

    set_json_headers(req);
    if (status != NULL) {
        httpd_resp_set_status(req, status);
    }
    esp_err_t err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);
    return err;
}

static bool has_ws_scheme(const char *url)
{
    if (url == NULL || url[0] == '\0') {
        return true;
    }
    return strncmp(url, "ws://", 5) == 0 || strncmp(url, "wss://", 6) == 0;
}

static bool normalize_country_code(char *country_code, size_t country_code_len)
{
    if (country_code == NULL || country_code_len < APP_WIFI_COUNTRY_CODE_MAX_LEN) {
        return false;
    }
    if (country_code[0] == '\0') {
        strlcpy(country_code, APP_WIFI_COUNTRY_CODE, country_code_len);
        return true;
    }
    if (strlen(country_code) != 2) {
        return false;
    }
    if (!isalpha((unsigned char)country_code[0]) || !isalpha((unsigned char)country_code[1])) {
        return false;
    }

    country_code[0] = (char)toupper((unsigned char)country_code[0]);
    country_code[1] = (char)toupper((unsigned char)country_code[1]);
    country_code[2] = '\0';
    return true;
}

static bool normalize_ui_language(char *language, size_t language_len)
{
    if (language == NULL || language_len == 0) {
        return false;
    }
    if (language[0] == '\0') {
        strlcpy(language, APP_UI_DEFAULT_LANGUAGE, language_len);
        return true;
    }

    char normalized[APP_UI_LANGUAGE_MAX_LEN] = {0};
    if (!i18n_store_normalize_language_code(language, normalized, sizeof(normalized))) {
        return false;
    }
    strlcpy(language, normalized, language_len);
    return true;
}

static void restart_timer_cb(void *arg)
{
    (void)arg;
    /* Avoid random panel colors during software reset. */
    (void)bsp_display_backlight_off();
    esp_restart();
}

static void schedule_restart(void)
{
    if (s_restart_timer == NULL) {
        const esp_timer_create_args_t timer_args = {
            .callback = &restart_timer_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "settings_restart",
            .skip_unhandled_events = true,
        };
        if (esp_timer_create(&timer_args, &s_restart_timer) != ESP_OK) {
            esp_restart();
            return;
        }
    }

    if (esp_timer_is_active(s_restart_timer)) {
        (void)esp_timer_stop(s_restart_timer);
    }
    if (esp_timer_start_once(s_restart_timer, 1500ULL * 1000ULL) != ESP_OK) {
        esp_restart();
    }
}

esp_err_t api_settings_get_handler(httpd_req_t *req)
{
    runtime_settings_t *settings = calloc(1, sizeof(runtime_settings_t));
    if (settings == NULL) {
        return httpd_resp_send_500(req);
    }

    esp_err_t settings_err = runtime_settings_load(settings);
    if (settings_err != ESP_OK) {
        runtime_settings_set_defaults(settings);
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *wifi = cJSON_CreateObject();
    cJSON *ha = cJSON_CreateObject();
    cJSON *time_cfg = cJSON_CreateObject();
    cJSON *ui = cJSON_CreateObject();
    if (root == NULL || wifi == NULL || ha == NULL || time_cfg == NULL || ui == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(wifi);
        cJSON_Delete(ha);
        cJSON_Delete(time_cfg);
        cJSON_Delete(ui);
        free(settings);
        return httpd_resp_send_500(req);
    }

    cJSON_AddStringToObject(wifi, "ssid", settings->wifi_ssid);
    cJSON_AddStringToObject(wifi, "country_code", settings->wifi_country_code);
    cJSON_AddBoolToObject(wifi, "password_set", settings->wifi_password[0] != '\0');
    cJSON_AddBoolToObject(wifi, "configured", runtime_settings_has_wifi(settings));
    cJSON_AddBoolToObject(wifi, "connected", wifi_mgr_is_connected());
    cJSON_AddBoolToObject(wifi, "setup_ap_active", wifi_mgr_is_setup_ap_active());
    cJSON_AddStringToObject(wifi, "setup_ap_ssid", wifi_mgr_get_setup_ap_ssid());
    int8_t wifi_rssi_dbm = 0;
    if (wifi_mgr_get_sta_rssi(&wifi_rssi_dbm) == ESP_OK) {
        cJSON_AddNumberToObject(wifi, "rssi_dbm", (double)wifi_rssi_dbm);
    } else {
        cJSON_AddNullToObject(wifi, "rssi_dbm");
    }
    cJSON_AddBoolToObject(wifi, "scan_supported", true);
    cJSON_AddItemToObject(root, "wifi", wifi);

    cJSON_AddStringToObject(ha, "ws_url", settings->ha_ws_url);
    cJSON_AddBoolToObject(ha, "access_token_set", settings->ha_access_token[0] != '\0');
    cJSON_AddBoolToObject(ha, "rest_enabled", settings->ha_rest_enabled);
    cJSON_AddBoolToObject(ha, "configured", runtime_settings_has_ha(settings));
    cJSON_AddBoolToObject(ha, "connected", ha_client_is_connected());
    cJSON_AddItemToObject(root, "ha", ha);

    cJSON_AddStringToObject(time_cfg, "ntp_server", settings->ntp_server);
    cJSON_AddStringToObject(time_cfg, "timezone", settings->time_tz);
    cJSON_AddItemToObject(root, "time", time_cfg);

    cJSON_AddStringToObject(ui, "language", settings->ui_language);
    cJSON_AddItemToObject(root, "ui", ui);

    cJSON_AddBoolToObject(root, "ok", true);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    free(settings);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }

    set_json_headers(req);
    esp_err_t err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);
    return err;
}

static bool update_string_setting(
    cJSON *obj,
    const char *key,
    char *dst,
    size_t dst_len,
    bool *out_invalid_type,
    bool *out_too_long)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL) {
        return false;
    }

    if (cJSON_IsString(item) && item->valuestring != NULL) {
        if (strlen(item->valuestring) >= dst_len) {
            if (out_too_long != NULL) {
                *out_too_long = true;
            }
            return false;
        }
        strlcpy(dst, item->valuestring, dst_len);
        return true;
    }
    if (cJSON_IsNull(item)) {
        dst[0] = '\0';
        return true;
    }

    if (out_invalid_type != NULL) {
        *out_invalid_type = true;
    }
    return false;
}

static bool update_bool_setting(cJSON *obj, const char *key, bool *dst, bool *out_invalid_type)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (item == NULL) {
        return false;
    }

    if (cJSON_IsBool(item)) {
        *dst = cJSON_IsTrue(item);
        return true;
    }

    if (out_invalid_type != NULL) {
        *out_invalid_type = true;
    }
    return false;
}

esp_err_t api_settings_put_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > APP_SETTINGS_MAX_JSON_LEN) {
        return send_json_error(req, "400 Bad Request", "Invalid payload size");
    }

    char *buf = calloc((size_t)req->content_len + 1U, sizeof(char));
    if (buf == NULL) {
        return httpd_resp_send_500(req);
    }

    int received = 0;
    while (received < req->content_len) {
        int r = httpd_req_recv(req, buf + received, req->content_len - received);
        if (r <= 0) {
            free(buf);
            return send_json_error(req, "400 Bad Request", "Failed to read request body");
        }
        received += r;
    }

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return send_json_error(req, "400 Bad Request", "Invalid JSON");
    }

    runtime_settings_t *settings = calloc(1, sizeof(runtime_settings_t));
    if (settings == NULL) {
        cJSON_Delete(root);
        return httpd_resp_send_500(req);
    }

    esp_err_t load_err = runtime_settings_load(settings);
    if (load_err != ESP_OK) {
        runtime_settings_set_defaults(settings);
    }

    cJSON *wifi = cJSON_GetObjectItemCaseSensitive(root, "wifi");
    cJSON *ha = cJSON_GetObjectItemCaseSensitive(root, "ha");
    cJSON *time_cfg = cJSON_GetObjectItemCaseSensitive(root, "time");
    cJSON *ui = cJSON_GetObjectItemCaseSensitive(root, "ui");
    if (wifi != NULL && !cJSON_IsObject(wifi)) {
        cJSON_Delete(root);
        free(settings);
        return send_json_error(req, "400 Bad Request", "wifi must be an object");
    }
    if (ha != NULL && !cJSON_IsObject(ha)) {
        cJSON_Delete(root);
        free(settings);
        return send_json_error(req, "400 Bad Request", "ha must be an object");
    }
    if (time_cfg != NULL && !cJSON_IsObject(time_cfg)) {
        cJSON_Delete(root);
        free(settings);
        return send_json_error(req, "400 Bad Request", "time must be an object");
    }
    if (ui != NULL && !cJSON_IsObject(ui)) {
        cJSON_Delete(root);
        free(settings);
        return send_json_error(req, "400 Bad Request", "ui must be an object");
    }

    bool invalid_type = false;
    bool too_long = false;
    if (cJSON_IsObject(wifi)) {
        (void)update_string_setting(
            wifi, "ssid", settings->wifi_ssid, sizeof(settings->wifi_ssid), &invalid_type, &too_long);
        (void)update_string_setting(
            wifi, "password", settings->wifi_password, sizeof(settings->wifi_password), &invalid_type, &too_long);
        (void)update_string_setting(
            wifi, "country_code", settings->wifi_country_code, sizeof(settings->wifi_country_code), &invalid_type, &too_long);
    }
    if (cJSON_IsObject(ha)) {
        (void)update_string_setting(
            ha, "ws_url", settings->ha_ws_url, sizeof(settings->ha_ws_url), &invalid_type, &too_long);
        (void)update_string_setting(
            ha, "access_token", settings->ha_access_token, sizeof(settings->ha_access_token), &invalid_type, &too_long);
        (void)update_bool_setting(ha, "rest_enabled", &settings->ha_rest_enabled, &invalid_type);
    }
    if (cJSON_IsObject(time_cfg)) {
        (void)update_string_setting(
            time_cfg, "ntp_server", settings->ntp_server, sizeof(settings->ntp_server), &invalid_type, &too_long);
        (void)update_string_setting(
            time_cfg, "timezone", settings->time_tz, sizeof(settings->time_tz), &invalid_type, &too_long);
    }
    if (cJSON_IsObject(ui)) {
        (void)update_string_setting(
            ui, "language", settings->ui_language, sizeof(settings->ui_language), &invalid_type, &too_long);
    }

    (void)update_string_setting(
        root, "wifi_ssid", settings->wifi_ssid, sizeof(settings->wifi_ssid), &invalid_type, &too_long);
    (void)update_string_setting(
        root, "wifi_password", settings->wifi_password, sizeof(settings->wifi_password), &invalid_type, &too_long);
    (void)update_string_setting(
        root, "wifi_country_code", settings->wifi_country_code, sizeof(settings->wifi_country_code), &invalid_type, &too_long);
    (void)update_string_setting(
        root, "ha_ws_url", settings->ha_ws_url, sizeof(settings->ha_ws_url), &invalid_type, &too_long);
    (void)update_string_setting(
        root, "ha_access_token", settings->ha_access_token, sizeof(settings->ha_access_token), &invalid_type, &too_long);
    (void)update_bool_setting(root, "ha_rest_enabled", &settings->ha_rest_enabled, &invalid_type);
    (void)update_string_setting(
        root, "ntp_server", settings->ntp_server, sizeof(settings->ntp_server), &invalid_type, &too_long);
    (void)update_string_setting(
        root, "time_tz", settings->time_tz, sizeof(settings->time_tz), &invalid_type, &too_long);
    (void)update_string_setting(
        root, "language", settings->ui_language, sizeof(settings->ui_language), &invalid_type, &too_long);

    bool reboot = true;
    cJSON *reboot_item = cJSON_GetObjectItemCaseSensitive(root, "reboot");
    if (reboot_item != NULL) {
        if (cJSON_IsBool(reboot_item)) {
            reboot = cJSON_IsTrue(reboot_item);
        } else {
            cJSON_Delete(root);
            free(settings);
            return send_json_error(req, "400 Bad Request", "reboot must be boolean");
        }
    }

    cJSON_Delete(root);

    if (invalid_type) {
        free(settings);
        return send_json_error(req, "400 Bad Request", "One or more settings fields have invalid type");
    }
    if (too_long) {
        free(settings);
        return send_json_error(
            req,
            "400 Bad Request",
            "One or more settings values are too long (ssid<=32, wifi_password<=64, country_code<=2, ws_url<=255, token<=511, ntp<=127, timezone<=127, language<=15)");
    }
    if (!has_ws_scheme(settings->ha_ws_url)) {
        free(settings);
        return send_json_error(req, "400 Bad Request", "ha.ws_url must start with ws:// or wss://");
    }
    if (!normalize_country_code(settings->wifi_country_code, sizeof(settings->wifi_country_code))) {
        free(settings);
        return send_json_error(req, "400 Bad Request", "wifi.country_code must be a 2-letter ISO code (e.g. US, DE)");
    }
    if (settings->wifi_ssid[0] == '\0') {
        settings->wifi_password[0] = '\0';
    }
    if (settings->ntp_server[0] == '\0') {
        strlcpy(settings->ntp_server, APP_NTP_SERVER, sizeof(settings->ntp_server));
    }
    if (settings->time_tz[0] == '\0') {
        strlcpy(settings->time_tz, APP_TIME_TZ, sizeof(settings->time_tz));
    }
    if (!normalize_ui_language(settings->ui_language, sizeof(settings->ui_language))) {
        free(settings);
        return send_json_error(req, "400 Bad Request", "ui.language must use [a-z0-9_-] and be 2-15 chars");
    }

    esp_err_t save_err = runtime_settings_save(settings);
    free(settings);
    if (save_err != ESP_OK) {
        return httpd_resp_send_500(req);
    }

    cJSON *resp = cJSON_CreateObject();
    if (resp == NULL) {
        return httpd_resp_send_500(req);
    }
    cJSON_AddBoolToObject(resp, "ok", true);
    cJSON_AddBoolToObject(resp, "rebooting", reboot);
    char *payload = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }

    set_json_headers(req);
    esp_err_t send_err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);

    if (send_err == ESP_OK && reboot) {
        schedule_restart();
    }
    return send_err;
}
