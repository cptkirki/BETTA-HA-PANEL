/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "api/api_routes.h"

#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "app_config.h"
#include "ha/ha_model.h"

#define API_ENTITIES_MAX_ITEMS_DEFAULT 128U
#define API_ENTITIES_MAX_ITEMS_MIN 8U
#define API_ENTITIES_MAX_ITEMS_MAX 128U

static void set_json_headers(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

static size_t parse_max_items(const char *value)
{
    if (value == NULL || value[0] == '\0') {
        return API_ENTITIES_MAX_ITEMS_DEFAULT;
    }

    char *end_ptr = NULL;
    long parsed = strtol(value, &end_ptr, 10);
    if (end_ptr == value || *end_ptr != '\0' || parsed <= 0) {
        return API_ENTITIES_MAX_ITEMS_DEFAULT;
    }
    if ((size_t)parsed < API_ENTITIES_MAX_ITEMS_MIN) {
        return API_ENTITIES_MAX_ITEMS_MIN;
    }
    if ((size_t)parsed > API_ENTITIES_MAX_ITEMS_MAX) {
        return API_ENTITIES_MAX_ITEMS_MAX;
    }
    return (size_t)parsed;
}

esp_err_t api_entities_get_handler(httpd_req_t *req)
{
    char domain[APP_MAX_NAME_LEN] = {0};
    char search[APP_MAX_NAME_LEN] = {0};
    char limit_raw[12] = {0};

    int query_len = httpd_req_get_url_query_len(req);
    if (query_len > 0) {
        char *query = calloc((size_t)query_len + 1U, sizeof(char));
        if (query == NULL) {
            return httpd_resp_send_500(req);
        }
        if (httpd_req_get_url_query_str(req, query, query_len + 1) == ESP_OK) {
            httpd_query_key_value(query, "domain", domain, sizeof(domain));
            httpd_query_key_value(query, "search", search, sizeof(search));
            httpd_query_key_value(query, "limit", limit_raw, sizeof(limit_raw));
        }
        free(query);
    }

    const size_t max_items = parse_max_items(limit_raw);
    ha_entity_info_t *items = calloc(max_items, sizeof(ha_entity_info_t));
    if (items == NULL) {
        return httpd_resp_send_500(req);
    }
    size_t count = ha_model_list_entities(
        domain[0] ? domain : NULL, search[0] ? search : NULL, items, max_items);

    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    if (root == NULL || arr == NULL) {
        free(items);
        cJSON_Delete(root);
        cJSON_Delete(arr);
        return httpd_resp_send_500(req);
    }

    for (size_t i = 0; i < count; i++) {
        cJSON *it = cJSON_CreateObject();
        cJSON_AddStringToObject(it, "id", items[i].id);
        cJSON_AddStringToObject(it, "name", items[i].name);
        cJSON_AddStringToObject(it, "domain", items[i].domain);
        cJSON_AddStringToObject(it, "unit", items[i].unit);
        cJSON_AddStringToObject(it, "device_class", items[i].device_class);
        cJSON_AddNumberToObject(it, "supported_features", (double)items[i].supported_features);
        cJSON_AddStringToObject(it, "icon", items[i].icon);
        cJSON_AddItemToArray(arr, it);
    }
    free(items);

    cJSON_AddItemToObject(root, "items", arr);
    cJSON_AddNumberToObject(root, "count", (double)count);
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
