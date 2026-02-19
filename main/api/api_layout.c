/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "api/api_routes.h"

#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"

#include "app_events.h"
#include "app_config.h"
#include "ha/ha_client.h"
#include "layout/layout_store.h"
#include "layout/layout_validate.h"
#include "util/log_tags.h"

static void set_json_headers(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

esp_err_t api_layout_get_handler(httpd_req_t *req)
{
    char *json = NULL;
    esp_err_t err = layout_store_load(&json);
    if (err != ESP_OK || json == NULL) {
        json = strdup(layout_store_default_json());
        if (json == NULL) {
            return httpd_resp_send_500(req);
        }
    }

    set_json_headers(req);
    esp_err_t send_err = httpd_resp_sendstr(req, json);
    free(json);
    return send_err;
}

static esp_err_t api_layout_send_validation_error(httpd_req_t *req, const layout_validation_result_t *validation)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *errors = cJSON_CreateArray();
    if (root == NULL || errors == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(errors);
        return httpd_resp_send_500(req);
    }
    cJSON_AddBoolToObject(root, "ok", false);
    for (uint16_t i = 0; i < validation->count; i++) {
        cJSON_AddItemToArray(errors, cJSON_CreateString(validation->messages[i]));
    }
    cJSON_AddItemToObject(root, "errors", errors);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }

    set_json_headers(req);
    httpd_resp_set_status(req, "400 Bad Request");
    esp_err_t err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);
    return err;
}

esp_err_t api_layout_put_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > APP_LAYOUT_MAX_JSON_LEN) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid payload size");
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
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to read body");
        }
        received += r;
    }

    layout_validation_result_t validation;
    bool valid = layout_validate_json(buf, &validation);
    if (!valid) {
        ESP_LOGW(TAG_LAYOUT, "Layout validation failed with %u errors", validation.count);
        esp_err_t err = api_layout_send_validation_error(req, &validation);
        free(buf);
        return err;
    }

    esp_err_t err = layout_store_save(buf);
    free(buf);
    if (err != ESP_OK) {
        return httpd_resp_send_500(req);
    }

    app_event_t event = {.type = EV_LAYOUT_UPDATED};
    app_events_publish(&event, pdMS_TO_TICKS(20));
    esp_err_t ha_notify_err = ha_client_notify_layout_updated();
    if (ha_notify_err != ESP_OK) {
        ESP_LOGW(TAG_LAYOUT, "Failed to notify HA client about layout update: %s", esp_err_to_name(ha_notify_err));
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return httpd_resp_send_500(req);
    }
    set_json_headers(req);
    err = httpd_resp_sendstr(req, payload);
    cJSON_free(payload);
    return err;
}
