#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t ui_bindings_toggle_entity(const char *entity_id);
esp_err_t ui_bindings_set_entity_power(const char *entity_id, bool on);
esp_err_t ui_bindings_set_slider_value(const char *entity_id, int value);
