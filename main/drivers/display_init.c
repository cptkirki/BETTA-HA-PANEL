#include "drivers/display_init.h"

#include <stdbool.h>

#include "bsp/display.h"
#include "bsp/esp32_p4_wifi6_touch_lcd_4b.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "app_config.h"
#include "util/log_tags.h"

#define DISPLAY_DRAW_BUFFER_PIXELS ((APP_SCREEN_WIDTH * APP_SCREEN_HEIGHT) / 5U)

static bool s_display_ready = false;
static lv_display_t *s_lv_display = NULL;

static lvgl_port_cfg_t display_port_cfg(void)
{
    lvgl_port_cfg_t cfg = ESP_LVGL_PORT_INIT_CONFIG();
    cfg.task_priority = 20;
    cfg.task_stack = APP_LVGL_TASK_STACK;
    cfg.task_affinity = 1;
    cfg.task_max_sleep_ms = 100;
    return cfg;
}

esp_err_t display_init(void)
{
    if (s_display_ready) {
        return ESP_OK;
    }

    lvgl_port_cfg_t lvgl_cfg = display_port_cfg();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_DISPLAY, "lvgl_port_init failed: %s", esp_err_to_name(err));
        return err;
    }

    bsp_lcd_handles_t lcd = {0};
    err = bsp_display_new_with_handles(NULL, &lcd);
    if (err != ESP_OK || lcd.panel == NULL) {
        ESP_LOGE(TAG_DISPLAY, "bsp_display_new_with_handles failed: %s", esp_err_to_name(err));
        return (err == ESP_OK) ? ESP_FAIL : err;
    }

    err = esp_lcd_panel_disp_on_off(lcd.panel, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG_DISPLAY, "Could not enable LCD panel output: %s", esp_err_to_name(err));
    }

    err = bsp_display_backlight_on();
    if (err != ESP_OK) {
        ESP_LOGW(TAG_DISPLAY, "Could not enable backlight: %s", esp_err_to_name(err));
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd.io,
        .panel_handle = lcd.panel,
        .control_handle = lcd.control,
        .buffer_size = DISPLAY_DRAW_BUFFER_PIXELS,
        .double_buffer = true,
        .hres = APP_SCREEN_WIDTH,
        .vres = APP_SCREEN_HEIGHT,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
#if LV_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
#if LV_VERSION_MAJOR >= 9
            .swap_bytes = (BSP_LCD_BIGENDIAN ? true : false),
#endif
            .full_refresh = false,
            .direct_mode = true,
        },
    };

    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags = {
            .avoid_tearing = true,
        },
    };

    s_lv_display = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    if (s_lv_display == NULL) {
        ESP_LOGE(TAG_DISPLAY, "lvgl_port_add_disp_dsi failed");
        return ESP_FAIL;
    }

    lv_display_set_antialiasing(s_lv_display, APP_LVGL_ANTIALIASING != 0);
    ESP_LOGI(TAG_DISPLAY, "LVGL antialiasing: %s", (APP_LVGL_ANTIALIASING != 0) ? "on" : "off");

    s_display_ready = true;
    ESP_LOGI(TAG_DISPLAY,
        "Display initialized (esp_lvgl_port + DSI, avoid_tearing=1, double_buffer=1, draw_buf=%u px)",
        (unsigned)DISPLAY_DRAW_BUFFER_PIXELS);
    return ESP_OK;
}

bool display_is_ready(void)
{
    return s_display_ready;
}

bool display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void display_unlock(void)
{
    lvgl_port_unlock();
}
