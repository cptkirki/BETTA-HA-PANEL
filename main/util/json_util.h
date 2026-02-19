/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>

#include "cJSON.h"

cJSON *json_util_parse(const char *json);
char *json_util_print_unformatted(const cJSON *json);
bool json_util_get_string(const cJSON *obj, const char *key, const char **out_value);
bool json_util_get_int(const cJSON *obj, const char *key, int *out_value);
void json_util_safe_free(char **ptr);
