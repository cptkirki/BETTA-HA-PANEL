/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "api/api_routes.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "app_config.h"
#include "settings/i18n_store.h"
#include "settings/runtime_settings.h"

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

static cJSON *parse_object_or_empty(const char *json)
{
    cJSON *root = cJSON_Parse((json != NULL) ? json : "{}");
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        root = cJSON_CreateObject();
    }
    return root;
}

static void merge_object(cJSON *dst, const cJSON *src)
{
    if (!cJSON_IsObject(dst) || !cJSON_IsObject(src)) {
        return;
    }

    for (const cJSON *child = src->child; child != NULL; child = child->next) {
        if (child->string == NULL || child->string[0] == '\0') {
            continue;
        }

        cJSON *dst_child = cJSON_GetObjectItemCaseSensitive(dst, child->string);
        if (cJSON_IsObject(child) && cJSON_IsObject(dst_child)) {
            merge_object(dst_child, child);
            continue;
        }

        cJSON *copy = cJSON_Duplicate(child, true);
        if (copy == NULL) {
            continue;
        }
        if (dst_child != NULL) {
            cJSON_DeleteItemFromObjectCaseSensitive(dst, child->string);
        }
        cJSON_AddItemToObject(dst, child->string, copy);
    }
}

static bool query_param_lang(httpd_req_t *req, char *out_language, size_t out_len)
{
    if (req == NULL || out_language == NULL || out_len == 0) {
        return false;
    }

    char query[96] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        return false;
    }

    char raw_lang[APP_UI_LANGUAGE_MAX_LEN * 2U] = {0};
    if (httpd_query_key_value(query, "lang", raw_lang, sizeof(raw_lang)) != ESP_OK) {
        return false;
    }

    return i18n_store_normalize_language_code(raw_lang, out_language, out_len);
}

static void selected_language_from_settings(char *out_language, size_t out_len)
{
    if (out_language == NULL || out_len == 0) {
        return;
    }
    out_language[0] = '\0';

    runtime_settings_t *settings = calloc(1, sizeof(runtime_settings_t));
    if (settings == NULL) {
        strlcpy(out_language, APP_UI_DEFAULT_LANGUAGE, out_len);
        return;
    }

    if (runtime_settings_load(settings) != ESP_OK) {
        runtime_settings_set_defaults(settings);
    }
    if (!i18n_store_normalize_language_code(settings->ui_language, out_language, out_len)) {
        strlcpy(out_language, APP_UI_DEFAULT_LANGUAGE, out_len);
    }
    free(settings);
}

static esp_err_t send_json_object(httpd_req_t *req, cJSON *root)
{
    if (req == NULL || root == NULL) {
        return httpd_resp_send_500(req);
    }
    char *payload = cJSON_PrintUnformatted(root);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }
    set_json_headers(req);
    esp_err_t err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);
    return err;
}

esp_err_t api_i18n_languages_get_handler(httpd_req_t *req)
{
    char (*languages)[APP_UI_LANGUAGE_MAX_LEN] = calloc(64, sizeof(*languages));
    if (languages == NULL) {
        return httpd_resp_send_500(req);
    }

    size_t language_count = 0;
    if (i18n_store_list_languages(languages, 64, &language_count) != ESP_OK) {
        free(languages);
        return httpd_resp_send_500(req);
    }

    char selected[APP_UI_LANGUAGE_MAX_LEN] = {0};
    selected_language_from_settings(selected, sizeof(selected));

    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    if (root == NULL || arr == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(arr);
        free(languages);
        return httpd_resp_send_500(req);
    }

    for (size_t i = 0; i < language_count; i++) {
        cJSON *entry = cJSON_CreateObject();
        if (entry == NULL) {
            continue;
        }
        const char *code = languages[i];
        cJSON_AddStringToObject(entry, "code", code);
        cJSON_AddBoolToObject(entry, "builtin", i18n_store_is_builtin_language(code));
        cJSON_AddBoolToObject(entry, "custom", i18n_store_custom_translation_exists(code));
        cJSON_AddItemToArray(arr, entry);
    }

    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddStringToObject(root, "selected", selected);
    cJSON_AddItemToObject(root, "languages", arr);

    esp_err_t err = send_json_object(req, root);
    cJSON_Delete(root);
    free(languages);
    return err;
}

esp_err_t api_i18n_effective_get_handler(httpd_req_t *req)
{
    char lang[APP_UI_LANGUAGE_MAX_LEN] = {0};
    if (!query_param_lang(req, lang, sizeof(lang))) {
        selected_language_from_settings(lang, sizeof(lang));
    }

    const char *builtin_json = i18n_store_builtin_translation_json(lang);
    if (builtin_json == NULL) {
        builtin_json = i18n_store_builtin_translation_json("en");
    }

    cJSON *root = parse_object_or_empty(builtin_json);
    if (root == NULL) {
        return httpd_resp_send_500(req);
    }

    char *custom_json = NULL;
    if (i18n_store_load_custom_translation(lang, &custom_json) == ESP_OK && custom_json != NULL) {
        cJSON *custom_root = parse_object_or_empty(custom_json);
        if (custom_root != NULL) {
            merge_object(root, custom_root);
            cJSON_Delete(custom_root);
        }
        free(custom_json);
    }

    cJSON *meta = cJSON_GetObjectItemCaseSensitive(root, "meta");
    if (!cJSON_IsObject(meta)) {
        meta = cJSON_CreateObject();
        if (meta == NULL) {
            cJSON_Delete(root);
            return httpd_resp_send_500(req);
        }
        cJSON_AddItemToObject(root, "meta", meta);
    }
    cJSON_DeleteItemFromObjectCaseSensitive(meta, "code");
    cJSON_AddStringToObject(meta, "code", lang);

    esp_err_t err = send_json_object(req, root);
    cJSON_Delete(root);
    return err;
}

esp_err_t api_i18n_custom_put_handler(httpd_req_t *req)
{
    char lang[APP_UI_LANGUAGE_MAX_LEN] = {0};
    if (!query_param_lang(req, lang, sizeof(lang))) {
        return send_json_error(req, "400 Bad Request", "lang query parameter is required");
    }

    if (req->content_len <= 0 || req->content_len > APP_I18N_MAX_JSON_LEN) {
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

    cJSON *json = cJSON_Parse(buf);
    free(buf);
    if (!cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return send_json_error(req, "400 Bad Request", "Invalid JSON object");
    }

    char *normalized_payload = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (normalized_payload == NULL) {
        return httpd_resp_send_500(req);
    }

    size_t payload_len = strlen(normalized_payload);
    esp_err_t save_err = i18n_store_save_custom_translation(lang, normalized_payload, payload_len);
    cJSON_free(normalized_payload);
    if (save_err != ESP_OK) {
        return httpd_resp_send_500(req);
    }

    cJSON *resp = cJSON_CreateObject();
    if (resp == NULL) {
        return httpd_resp_send_500(req);
    }
    cJSON_AddBoolToObject(resp, "ok", true);
    cJSON_AddStringToObject(resp, "lang", lang);
    esp_err_t err = send_json_object(req, resp);
    cJSON_Delete(resp);
    return err;
}
