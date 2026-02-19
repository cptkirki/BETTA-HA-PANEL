/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    UI_BINDINGS_MEDIA_ACTION_PLAY_PAUSE = 0,
    UI_BINDINGS_MEDIA_ACTION_STOP,
    UI_BINDINGS_MEDIA_ACTION_NEXT,
    UI_BINDINGS_MEDIA_ACTION_PREVIOUS,
} ui_bindings_media_action_t;

esp_err_t ui_bindings_toggle_entity(const char *entity_id);
esp_err_t ui_bindings_set_entity_power(const char *entity_id, bool on);
esp_err_t ui_bindings_set_slider_value(const char *entity_id, int value);
esp_err_t ui_bindings_media_player_action(const char *entity_id, ui_bindings_media_action_t action);
