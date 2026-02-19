/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app_config.h"

typedef enum {
    EV_NONE = 0,
    EV_HA_CONNECTED,
    EV_HA_DISCONNECTED,
    EV_HA_STATE_CHANGED,
    EV_LAYOUT_UPDATED,
    EV_UI_NAVIGATE,
} app_event_type_t;

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
} app_event_state_changed_t;

typedef struct {
    char page_id[APP_MAX_PAGE_ID_LEN];
} app_event_navigate_t;

typedef struct {
    app_event_type_t type;
    union {
        app_event_state_changed_t ha_state_changed;
        app_event_navigate_t navigate;
    } data;
} app_event_t;

esp_err_t app_events_init(void);
QueueHandle_t app_events_get_queue(void);
bool app_events_publish(const app_event_t *event, TickType_t timeout_ticks);
bool app_events_receive(app_event_t *event, TickType_t timeout_ticks);
