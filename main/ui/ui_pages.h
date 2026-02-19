/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "lvgl.h"

void ui_pages_init(void);
void ui_pages_reset(void);
lv_obj_t *ui_pages_add(const char *page_id, const char *title);
bool ui_pages_show(const char *page_id);
bool ui_pages_show_index(uint16_t index);
bool ui_pages_next(void);
const char *ui_pages_current_id(void);
uint16_t ui_pages_count(void);
void ui_pages_set_topbar_status(
    bool wifi_connected, bool wifi_setup_ap_active, bool api_connected, bool api_initial_sync_done);
void ui_pages_set_topbar_datetime(const struct tm *timeinfo);
