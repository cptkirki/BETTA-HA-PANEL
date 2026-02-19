/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "ui/ui_i18n.h"

#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "app_config.h"
#include "settings/i18n_store.h"

static cJSON *s_translation_root = NULL;
static char s_language[APP_UI_LANGUAGE_MAX_LEN] = APP_UI_DEFAULT_LANGUAGE;

static cJSON *ui_i18n_parse_object_or_empty(const char *json)
{
    cJSON *root = cJSON_Parse(json != NULL ? json : "{}");
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        root = cJSON_CreateObject();
    }
    return root;
}

static void ui_i18n_merge_object(cJSON *dst, const cJSON *src)
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
            ui_i18n_merge_object(dst_child, child);
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

esp_err_t ui_i18n_init(const char *language_code)
{
    char normalized[APP_UI_LANGUAGE_MAX_LEN] = {0};
    if (!i18n_store_normalize_language_code(language_code, normalized, sizeof(normalized))) {
        strlcpy(normalized, APP_UI_DEFAULT_LANGUAGE, sizeof(normalized));
    }

    const char *builtin_json = i18n_store_builtin_translation_json(normalized);
    if (builtin_json == NULL) {
        builtin_json = i18n_store_builtin_translation_json("en");
    }

    cJSON *next_root = ui_i18n_parse_object_or_empty(builtin_json);
    if (next_root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    char *custom_json = NULL;
    if (i18n_store_load_custom_translation(normalized, &custom_json) == ESP_OK && custom_json != NULL) {
        cJSON *custom_root = ui_i18n_parse_object_or_empty(custom_json);
        if (custom_root != NULL) {
            ui_i18n_merge_object(next_root, custom_root);
            cJSON_Delete(custom_root);
        }
        free(custom_json);
    }

    cJSON_Delete(s_translation_root);
    s_translation_root = next_root;
    strlcpy(s_language, normalized, sizeof(s_language));
    return ESP_OK;
}

void ui_i18n_reset(void)
{
    cJSON_Delete(s_translation_root);
    s_translation_root = NULL;
    strlcpy(s_language, APP_UI_DEFAULT_LANGUAGE, sizeof(s_language));
}

static const cJSON *ui_i18n_lookup_path(const char *key)
{
    if (!cJSON_IsObject(s_translation_root) || key == NULL || key[0] == '\0') {
        return NULL;
    }

    const cJSON *node = cJSON_GetObjectItemCaseSensitive(s_translation_root, "lvgl");
    if (!cJSON_IsObject(node)) {
        return NULL;
    }

    const char *segment_start = key;
    while (segment_start != NULL && segment_start[0] != '\0') {
        const char *dot = strchr(segment_start, '.');
        size_t len = (dot != NULL) ? (size_t)(dot - segment_start) : strlen(segment_start);
        if (len == 0 || len >= 64) {
            return NULL;
        }

        char segment[64] = {0};
        memcpy(segment, segment_start, len);
        segment[len] = '\0';

        node = cJSON_GetObjectItemCaseSensitive((cJSON *)node, segment);
        if (node == NULL) {
            return NULL;
        }
        if (dot == NULL) {
            return node;
        }
        segment_start = dot + 1;
    }

    return node;
}

const char *ui_i18n_get(const char *key, const char *fallback)
{
    const cJSON *node = ui_i18n_lookup_path(key);
    if (cJSON_IsString(node) && node->valuestring != NULL && node->valuestring[0] != '\0') {
        return node->valuestring;
    }
    return (fallback != NULL) ? fallback : "";
}

const char *ui_i18n_get_language(void)
{
    return s_language;
}
