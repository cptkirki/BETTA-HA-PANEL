/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t touch_init(void);
bool touch_is_ready(void);
