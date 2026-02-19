/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t ui_boot_splash_show(void);
void ui_boot_splash_set_title(const char *title_text);
void ui_boot_splash_clear_status(void);
void ui_boot_splash_set_status(const char *status_text);
void ui_boot_splash_set_status_layout(bool centered, uint16_t width, int16_t x_offset);
void ui_boot_splash_set_status_x_offset(int16_t x_offset);
void ui_boot_splash_set_progress(uint8_t progress_percent);
void ui_boot_splash_hide(void);
