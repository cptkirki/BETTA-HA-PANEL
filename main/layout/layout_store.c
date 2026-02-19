/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "layout/layout_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "app_config.h"
#include "layout/layout_validate.h"
#include "util/log_tags.h"

static const char *s_default_layout =
    "{"
    "\"version\":1,"
    "\"pages\":["
    "{"
    "\"id\":\"wohnen\","
    "\"title\":\"Wohnen\","
    "\"widgets\":["
    "{"
    "\"id\":\"light_stehlampe_2\","
    "\"type\":\"light_tile\","
    "\"title\":\"Stehlampe\","
    "\"entity_id\":\"light.stehlampe_2_tradfri_bulb_e14_ws_470lm_light\","
    "\"rect\":{\"x\":185,\"y\":160,\"w\":320,\"h\":280}"
    "}"
    "]"
    "}"
    "]"
    "}";

esp_err_t layout_store_save(const char *json)
{
    if (json == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *f = fopen(APP_LAYOUT_PATH, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG_LAYOUT, "Cannot open layout file for writing: %s", APP_LAYOUT_PATH);
        return ESP_FAIL;
    }

    size_t len = strlen(json);
    size_t written = fwrite(json, 1U, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG_LAYOUT, "Failed to write layout file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_LAYOUT, "Saved layout (%u bytes)", (unsigned)len);
    return ESP_OK;
}

esp_err_t layout_store_load(char **json_out)
{
    if (json_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *json_out = NULL;

    FILE *f = fopen(APP_LAYOUT_PATH, "rb");
    if (f == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return ESP_FAIL;
    }
    long size = ftell(f);
    if (size <= 0 || size > APP_LAYOUT_MAX_JSON_LEN) {
        fclose(f);
        return ESP_ERR_INVALID_SIZE;
    }
    rewind(f);

    char *buf = (char *)calloc((size_t)size + 1U, sizeof(char));
    if (buf == NULL) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    size_t read = fread(buf, 1U, (size_t)size, f);
    fclose(f);
    if (read != (size_t)size) {
        free(buf);
        return ESP_FAIL;
    }

    *json_out = buf;
    return ESP_OK;
}

const char *layout_store_default_json(void)
{
    return s_default_layout;
}

esp_err_t layout_store_init(void)
{
    char *existing = NULL;
    esp_err_t err = layout_store_load(&existing);
    if (err != ESP_OK || existing == NULL) {
        ESP_LOGW(TAG_LAYOUT, "Layout file missing or invalid, writing defaults");
        return layout_store_save(s_default_layout);
    }

    layout_validation_result_t *validation = calloc(1, sizeof(layout_validation_result_t));
    if (validation == NULL) {
        free(existing);
        ESP_LOGW(TAG_LAYOUT, "Validation allocation failed, preserving existing layout");
        return ESP_OK;
    }

    bool valid = layout_validate_json(existing, validation);
    free(validation);
    free(existing);

    if (valid) {
        return ESP_OK;
    }

    ESP_LOGW(TAG_LAYOUT, "Stored layout failed validation, writing defaults");
    return layout_store_save(s_default_layout);
}
