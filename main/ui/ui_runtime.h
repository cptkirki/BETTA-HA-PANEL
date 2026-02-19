/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include "esp_err.h"

esp_err_t ui_runtime_init(void);
esp_err_t ui_runtime_load_layout(const char *layout_json);
esp_err_t ui_runtime_reload_layout(void);
esp_err_t ui_runtime_start(void);
