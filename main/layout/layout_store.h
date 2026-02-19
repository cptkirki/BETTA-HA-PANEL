/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include "esp_err.h"

esp_err_t layout_store_init(void);
esp_err_t layout_store_load(char **json_out);
esp_err_t layout_store_save(const char *json);
const char *layout_store_default_json(void);
