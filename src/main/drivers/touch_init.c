#include "drivers/touch_init.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp/touch.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "drivers/display_init.h"
#include "util/log_tags.h"

static bool s_touch_ready = false;
static lv_indev_t *s_touch_indev = NULL;
static esp_lcd_touch_handle_t s_touch_handle = NULL;
static const int TOUCH_INIT_RETRIES = 8;
static const int TOUCH_INIT_RETRY_DELAY_MS = 250;

static lv_display_t *touch_get_display(void)
{
#if LV_VERSION_MAJOR >= 9
    return lv_display_get_default();
#else
    return lv_disp_get_default();
#endif
}

esp_err_t touch_init(void)
{
    if (s_touch_ready) {
        return ESP_OK;
    }
    if (!display_is_ready()) {
        return ESP_ERR_INVALID_STATE;
    }

    lv_display_t *disp = touch_get_display();
    if (disp == NULL) {
        ESP_LOGE(TAG_TOUCH, "No active LVGL display for touch binding");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = ESP_FAIL;
    for (int attempt = 1; attempt <= TOUCH_INIT_RETRIES; attempt++) {
        err = bsp_touch_new(NULL, &s_touch_handle);
        if (err == ESP_OK && s_touch_handle != NULL) {
            break;
        }
        ESP_LOGW(TAG_TOUCH, "bsp_touch_new attempt %d/%d failed: %s",
            attempt, TOUCH_INIT_RETRIES, esp_err_to_name(err));
        if (attempt < TOUCH_INIT_RETRIES) {
            vTaskDelay(pdMS_TO_TICKS(TOUCH_INIT_RETRY_DELAY_MS));
        }
    }
    if (err != ESP_OK || s_touch_handle == NULL) {
        ESP_LOGE(TAG_TOUCH, "bsp_touch_new failed after %d attempts: %s",
            TOUCH_INIT_RETRIES, esp_err_to_name(err));
        return (err == ESP_OK) ? ESP_FAIL : err;
    }

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = s_touch_handle,
        .scale = {
            .x = 1.0f,
            .y = 1.0f,
        },
    };

    if (!display_lock(1000)) {
        esp_lcd_touch_del(s_touch_handle);
        s_touch_handle = NULL;
        return ESP_ERR_TIMEOUT;
    }
    s_touch_indev = lvgl_port_add_touch(&touch_cfg);
    display_unlock();

    if (s_touch_indev == NULL) {
        ESP_LOGE(TAG_TOUCH, "lvgl_port_add_touch failed");
        esp_lcd_touch_del(s_touch_handle);
        s_touch_handle = NULL;
        return ESP_FAIL;
    }

    s_touch_ready = true;
    ESP_LOGI(TAG_TOUCH, "Touch initialized (esp_lvgl_port + GT911)");
    return ESP_OK;
}

bool touch_is_ready(void)
{
    return s_touch_ready;
}
