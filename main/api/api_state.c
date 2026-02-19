/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "api/api_routes.h"

#include <stdlib.h>

#include "cJSON.h"

#include "app_config.h"
#include "ha/ha_model.h"

static void set_json_headers(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

static cJSON *state_to_json(const ha_state_t *state)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "entity_id", state->entity_id);
    cJSON_AddStringToObject(obj, "state", state->state);
    cJSON_AddStringToObject(obj, "attributes_json", state->attributes_json);
    cJSON_AddNumberToObject(obj, "last_changed_unix_ms", (double)state->last_changed_unix_ms);
    return obj;
}

esp_err_t api_state_get_handler(httpd_req_t *req)
{
    char entity_id[APP_MAX_ENTITY_ID_LEN] = {0};
    int query_len = httpd_req_get_url_query_len(req);
    if (query_len > 0) {
        char *query = calloc((size_t)query_len + 1U, sizeof(char));
        if (query == NULL) {
            return httpd_resp_send_500(req);
        }
        if (httpd_req_get_url_query_str(req, query, query_len + 1) == ESP_OK) {
            httpd_query_key_value(query, "entity_id", entity_id, sizeof(entity_id));
        }
        free(query);
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();
    if (root == NULL || items == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(items);
        return httpd_resp_send_500(req);
    }

    size_t count = 0;
    if (entity_id[0] != '\0') {
        ha_state_t state = {0};
        if (ha_model_get_state(entity_id, &state)) {
            cJSON_AddItemToArray(items, state_to_json(&state));
            count = 1;
        }
    } else {
        const size_t max_items = 128;
        ha_state_t *states = calloc(max_items, sizeof(ha_state_t));
        if (states == NULL) {
            cJSON_Delete(root);
            cJSON_Delete(items);
            return httpd_resp_send_500(req);
        }
        count = ha_model_list_states(states, max_items);
        for (size_t i = 0; i < count; i++) {
            cJSON_AddItemToArray(items, state_to_json(&states[i]));
        }
        free(states);
    }

    cJSON_AddItemToObject(root, "items", items);
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
