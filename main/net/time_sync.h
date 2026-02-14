#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t time_sync_set_timezone(const char *tz);
esp_err_t time_sync_start(const char *ntp_server);
bool time_sync_wait_for_sync(uint32_t timeout_ms);
