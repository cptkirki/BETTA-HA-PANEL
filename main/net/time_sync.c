/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "net/time_sync.h"

#include <stdlib.h>
#include <time.h>

#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "util/log_tags.h"

esp_err_t time_sync_set_timezone(const char *tz)
{
    const char *tz_value = (tz != NULL && tz[0] != '\0') ? tz : APP_TIME_TZ;
    if (setenv("TZ", tz_value, 1) != 0) {
        ESP_LOGW(TAG_TIME, "Failed to set TZ='%s'", tz_value);
        return ESP_FAIL;
    }
    tzset();
    ESP_LOGI(TAG_TIME, "Timezone set: %s", tz_value);
    return ESP_OK;
}

esp_err_t time_sync_start(const char *ntp_server)
{
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }

    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, ntp_server != NULL ? ntp_server : "pool.ntp.org");
    esp_sntp_init();
    ESP_LOGI(TAG_TIME, "SNTP started");
    return ESP_OK;
}

bool time_sync_wait_for_sync(uint32_t timeout_ms)
{
    const uint32_t step_ms = 500;
    uint32_t waited = 0;
    while (waited < timeout_ms) {
        time_t now = 0;
        struct tm info = {0};
        time(&now);
        localtime_r(&now, &info);
        if (info.tm_year > (2016 - 1900)) {
            ESP_LOGI(TAG_TIME, "Time synchronized");
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        waited += step_ms;
    }
    ESP_LOGW(TAG_TIME, "Time synchronization timeout");
    return false;
}
