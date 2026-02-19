/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"

typedef struct {
    uint16_t count;
    char messages[APP_LAYOUT_MAX_ERRORS][96];
} layout_validation_result_t;

void layout_validation_clear(layout_validation_result_t *result);
void layout_validation_add(layout_validation_result_t *result, const char *msg);
bool layout_validate_json(const char *json, layout_validation_result_t *result);
