#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t touch_init(void);
bool touch_is_ready(void);
