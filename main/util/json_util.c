/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "util/json_util.h"

#include <stdlib.h>

cJSON *json_util_parse(const char *json)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_Parse(json);
}

char *json_util_print_unformatted(const cJSON *json)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_PrintUnformatted(json);
}

bool json_util_get_string(const cJSON *obj, const char *key, const char **out_value)
{
    if (obj == NULL || key == NULL || out_value == NULL) {
        return false;
    }
    cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return false;
    }
    *out_value = item->valuestring;
    return true;
}

bool json_util_get_int(const cJSON *obj, const char *key, int *out_value)
{
    if (obj == NULL || key == NULL || out_value == NULL) {
        return false;
    }
    cJSON *item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!cJSON_IsNumber(item)) {
        return false;
    }
    *out_value = item->valueint;
    return true;
}

void json_util_safe_free(char **ptr)
{
    if (ptr == NULL || *ptr == NULL) {
        return;
    }
    free(*ptr);
    *ptr = NULL;
}
