#pragma once

#include "esp_err.h"

esp_err_t ui_bindings_toggle_entity(const char *entity_id);
esp_err_t ui_bindings_set_slider_value(const char *entity_id, int value);
