#include "ui/ui_boot_splash.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

#include "app_config.h"
#include "drivers/display_init.h"
#include "ui/ui_i18n.h"
#include "util/log_tags.h"

#if defined(__has_include)
#if __has_include("ui/assets/splash_house_image.h")
#include "ui/assets/splash_house_image.h"
#define APP_HAVE_SPLASH_HOUSE_IMAGE 1
#endif
#endif

#ifndef APP_HAVE_SPLASH_HOUSE_IMAGE
#define APP_HAVE_SPLASH_HOUSE_IMAGE 0
#endif

#ifndef APP_HAVE_SMART86OS_BETTA_IMAGE
#define APP_HAVE_SMART86OS_BETTA_IMAGE 0
#endif

#if APP_HAVE_SMART86OS_BETTA_IMAGE
extern const lv_image_dsc_t SMART86OS_Betta;
#endif

typedef struct {
    lv_obj_t *root;
    lv_obj_t *emblem;
    lv_obj_t *progress;
    lv_obj_t *title;
    lv_obj_t *status;
    lv_timer_t *timer;
    uint8_t progress_value;
    uint8_t status_line_count;
    int64_t shown_at_ms;
    char status_lines[5][64];
} boot_splash_state_t;

static boot_splash_state_t s_splash = {0};
static const int64_t BOOT_SPLASH_MIN_SHOW_MS = 1200;
#define SPLASH_ACCENT_HEX 0x38F2FF
#define SPLASH_BG_SOLID_HEX 0x000000
#define SPLASH_PROGRESS_BG_HEX 0x2A2F34
#define SPLASH_TITLE_HEX 0xF4F7FA
#define SPLASH_STATUS_HEX 0x8D98A5
#define SPLASH_PROGRESS_STEP 5
#define SPLASH_STATUS_MAX_LINES 5
#define SPLASH_STATUS_LINE_LEN 64
#define SPLASH_STATUS_X_OFFSET_DEFAULT 24
#define SPLASH_STATUS_WIDTH_DEFAULT 400
static int16_t s_status_x_offset = SPLASH_STATUS_X_OFFSET_DEFAULT;
static lv_coord_t s_status_width = SPLASH_STATUS_WIDTH_DEFAULT;
static lv_text_align_t s_status_text_align = LV_TEXT_ALIGN_LEFT;

static int64_t splash_now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static void splash_render_status_lines(void)
{
    if (s_splash.status == NULL) {
        return;
    }

    char text[(SPLASH_STATUS_MAX_LINES * SPLASH_STATUS_LINE_LEN) + SPLASH_STATUS_MAX_LINES] = {0};
    size_t offset = 0;
    for (uint8_t i = 0; i < s_splash.status_line_count; ++i) {
        int written = snprintf(
            text + offset,
            sizeof(text) - offset,
            "%s%s",
            s_splash.status_lines[i],
            (i + 1U < s_splash.status_line_count) ? "\n" : "");
        if (written <= 0) {
            break;
        }
        size_t w = (size_t)written;
        if (w >= (sizeof(text) - offset)) {
            offset = sizeof(text) - 1U;
            break;
        }
        offset += w;
    }

    lv_label_set_text(s_splash.status, text);
    lv_obj_set_width(s_splash.status, s_status_width);
    lv_obj_set_style_text_align(s_splash.status, s_status_text_align, LV_PART_MAIN);
    if (s_splash.title != NULL) {
        lv_obj_align_to(s_splash.status, s_splash.title, LV_ALIGN_OUT_BOTTOM_MID, s_status_x_offset, 14);
    }
}

static void splash_add_status_line(const char *status_text)
{
    if (status_text == NULL || status_text[0] == '\0') {
        return;
    }

    if (s_splash.status_line_count < SPLASH_STATUS_MAX_LINES) {
        snprintf(
            s_splash.status_lines[s_splash.status_line_count],
            sizeof(s_splash.status_lines[s_splash.status_line_count]),
            "%s",
            status_text);
        s_splash.status_line_count++;
    } else {
        for (uint8_t i = 0; i + 1U < SPLASH_STATUS_MAX_LINES; ++i) {
            memmove(
                s_splash.status_lines[i],
                s_splash.status_lines[i + 1U],
                sizeof(s_splash.status_lines[i]));
        }
        snprintf(
            s_splash.status_lines[SPLASH_STATUS_MAX_LINES - 1U],
            sizeof(s_splash.status_lines[SPLASH_STATUS_MAX_LINES - 1U]),
            "%s",
            status_text);
    }

    splash_render_status_lines();
}

static void splash_set_progress_value(uint8_t value)
{
    if (s_splash.progress == NULL) {
        return;
    }
    s_splash.progress_value = value;
    lv_bar_set_value(s_splash.progress, (int)value, LV_ANIM_OFF);
}

static void splash_step_progress(void)
{
    if (s_splash.progress == NULL) {
        return;
    }

    int value = (int)s_splash.progress_value + SPLASH_PROGRESS_STEP;
    if (value >= 100) {
        value = 0;
    }

    splash_set_progress_value((uint8_t)value);
}

static void splash_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    splash_step_progress();
}

static lv_obj_t *splash_create_emblem(lv_obj_t *parent)
{
    lv_obj_t *emblem = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(emblem, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_width(emblem, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(emblem, 0, LV_PART_MAIN);
    lv_obj_clear_flag(emblem, LV_OBJ_FLAG_SCROLLABLE);

#if APP_HAVE_SMART86OS_BETTA_IMAGE
    lv_obj_set_size(emblem, 370, 340);
    lv_obj_align(emblem, LV_ALIGN_TOP_MID, 0, 66);
    lv_obj_t *img = lv_image_create(emblem);
    lv_image_set_src(img, &SMART86OS_Betta);
    lv_obj_center(img);
#elif APP_HAVE_SPLASH_HOUSE_IMAGE
    lv_obj_set_size(emblem, 220, 220);
    lv_obj_align(emblem, LV_ALIGN_TOP_MID, 0, 110);
    lv_obj_t *img = lv_image_create(emblem);
    lv_image_set_src(img, &splash_house_image);
    lv_obj_center(img);
#else
    lv_obj_set_size(emblem, 360, 220);
    lv_obj_align(emblem, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_t *fallback = lv_label_create(emblem);
    lv_label_set_text(fallback, "SMART86");
    lv_obj_set_style_text_color(fallback, lv_color_hex(SPLASH_ACCENT_HEX), LV_PART_MAIN);
#if LV_FONT_MONTSERRAT_48
    lv_obj_set_style_text_font(fallback, &lv_font_montserrat_48, LV_PART_MAIN);
#endif
    lv_obj_center(fallback);
#endif

    return emblem;
}

esp_err_t ui_boot_splash_show(void)
{
    if (!display_is_ready()) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!display_lock(200)) {
        return ESP_ERR_TIMEOUT;
    }

    if (s_splash.root != NULL) {
        display_unlock();
        return ESP_OK;
    }

    lv_obj_t *top_layer = lv_layer_top();
    s_splash.root = lv_obj_create(top_layer);
    lv_obj_set_size(s_splash.root, APP_SCREEN_WIDTH, APP_SCREEN_HEIGHT);
    lv_obj_set_pos(s_splash.root, 0, 0);
    lv_obj_clear_flag(s_splash.root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_splash.root, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_splash.root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_splash.root, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_splash.root, lv_color_hex(SPLASH_BG_SOLID_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(s_splash.root, lv_color_hex(SPLASH_BG_SOLID_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(s_splash.root, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_move_foreground(s_splash.root);

    s_splash.emblem = splash_create_emblem(s_splash.root);

    s_splash.progress = lv_bar_create(s_splash.root);
    lv_obj_set_size(s_splash.progress, 340, 10);
    lv_obj_align_to(s_splash.progress, s_splash.emblem, LV_ALIGN_OUT_BOTTOM_MID, 0, 24);
    lv_bar_set_range(s_splash.progress, 0, 100);
    lv_bar_set_value(s_splash.progress, 20, LV_ANIM_OFF);
    lv_obj_set_style_radius(s_splash.progress, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_splash.progress, 0, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(s_splash.progress, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_splash.progress, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_splash.progress, lv_color_hex(SPLASH_PROGRESS_BG_HEX), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_splash.progress, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_splash.progress, lv_color_hex(SPLASH_ACCENT_HEX), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_splash.progress, LV_OPA_COVER, LV_PART_INDICATOR);

    s_splash.title = lv_label_create(s_splash.root);
    lv_label_set_text(s_splash.title, "SMART86 OS");
    lv_obj_set_style_text_color(s_splash.title, lv_color_hex(SPLASH_TITLE_HEX), LV_PART_MAIN);
#if LV_FONT_MONTSERRAT_48
    lv_obj_set_style_text_font(s_splash.title, &lv_font_montserrat_48, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_40
    lv_obj_set_style_text_font(s_splash.title, &lv_font_montserrat_40, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_34
    lv_obj_set_style_text_font(s_splash.title, &lv_font_montserrat_34, LV_PART_MAIN);
#endif
    lv_obj_align_to(s_splash.title, s_splash.progress, LV_ALIGN_OUT_BOTTOM_MID, 0, 18);

    s_splash.status = lv_label_create(s_splash.root);
    s_status_x_offset = SPLASH_STATUS_X_OFFSET_DEFAULT;
    s_status_width = SPLASH_STATUS_WIDTH_DEFAULT;
    s_status_text_align = LV_TEXT_ALIGN_LEFT;
    lv_obj_set_style_text_color(s_splash.status, lv_color_hex(SPLASH_STATUS_HEX), LV_PART_MAIN);
#if LV_FONT_MONTSERRAT_24
    lv_obj_set_style_text_font(s_splash.status, &lv_font_montserrat_24, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(s_splash.status, &lv_font_montserrat_20, LV_PART_MAIN);
#endif
    lv_obj_set_width(s_splash.status, s_status_width);
    lv_obj_set_style_text_align(s_splash.status, s_status_text_align, LV_PART_MAIN);
    lv_obj_align_to(s_splash.status, s_splash.title, LV_ALIGN_OUT_BOTTOM_MID, s_status_x_offset, 14);

    s_splash.status_line_count = 0;
    splash_add_status_line(ui_i18n_get("boot.initializing_system", "Initializing system"));
    s_splash.progress_value = 0;
    s_splash.timer = lv_timer_create(splash_timer_cb, 280, NULL);
    s_splash.shown_at_ms = splash_now_ms();

    display_unlock();
    ESP_LOGI(TAG_UI, "Boot splash shown");
    return ESP_OK;
}

void ui_boot_splash_set_status(const char *status_text)
{
    if (status_text == NULL || status_text[0] == '\0') {
        return;
    }
    if (s_splash.root == NULL || s_splash.status == NULL) {
        return;
    }
    if (!display_lock(50)) {
        return;
    }
    splash_add_status_line(status_text);
    display_unlock();
}

void ui_boot_splash_set_title(const char *title_text)
{
    if (title_text == NULL || title_text[0] == '\0') {
        return;
    }
    if (s_splash.root == NULL || s_splash.title == NULL) {
        return;
    }
    if (!display_lock(50)) {
        return;
    }
    lv_label_set_text(s_splash.title, title_text);
    if (s_splash.progress != NULL) {
        lv_obj_align_to(s_splash.title, s_splash.progress, LV_ALIGN_OUT_BOTTOM_MID, 0, 18);
    }
    if (s_splash.status != NULL) {
        lv_obj_align_to(s_splash.status, s_splash.title, LV_ALIGN_OUT_BOTTOM_MID, s_status_x_offset, 14);
    }
    display_unlock();
}

void ui_boot_splash_clear_status(void)
{
    if (s_splash.root == NULL || s_splash.status == NULL) {
        return;
    }
    if (!display_lock(50)) {
        return;
    }
    s_splash.status_line_count = 0;
    memset(s_splash.status_lines, 0, sizeof(s_splash.status_lines));
    splash_render_status_lines();
    display_unlock();
}

void ui_boot_splash_set_progress(uint8_t progress_percent)
{
    if (s_splash.root == NULL || s_splash.progress == NULL) {
        return;
    }
    if (!display_lock(50)) {
        return;
    }

    if (s_splash.timer != NULL) {
        lv_timer_del(s_splash.timer);
        s_splash.timer = NULL;
    }
    if (progress_percent > 100U) {
        progress_percent = 100U;
    }
    splash_set_progress_value(progress_percent);
    display_unlock();
}

void ui_boot_splash_set_status_x_offset(int16_t x_offset)
{
    if (s_splash.root == NULL || s_splash.status == NULL) {
        return;
    }
    if (!display_lock(50)) {
        return;
    }
    s_status_x_offset = x_offset;
    splash_render_status_lines();
    display_unlock();
}

void ui_boot_splash_set_status_layout(bool centered, uint16_t width, int16_t x_offset)
{
    if (s_splash.root == NULL || s_splash.status == NULL) {
        return;
    }
    if (!display_lock(50)) {
        return;
    }

    if (width >= 120U && width <= (uint16_t)APP_SCREEN_WIDTH) {
        s_status_width = (lv_coord_t)width;
    }
    s_status_text_align = centered ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT;
    s_status_x_offset = x_offset;
    splash_render_status_lines();
    display_unlock();
}

void ui_boot_splash_hide(void)
{
    if (s_splash.root == NULL) {
        return;
    }

    int64_t now_ms = splash_now_ms();
    int64_t elapsed = now_ms - s_splash.shown_at_ms;
    if (elapsed < BOOT_SPLASH_MIN_SHOW_MS) {
        int64_t remaining = BOOT_SPLASH_MIN_SHOW_MS - elapsed;
        vTaskDelay(pdMS_TO_TICKS((uint32_t)remaining));
    }

    if (!display_lock(200)) {
        return;
    }

    if (s_splash.timer != NULL) {
        lv_timer_del(s_splash.timer);
        s_splash.timer = NULL;
    }
    if (s_splash.root != NULL) {
        lv_obj_del(s_splash.root);
    }
    memset(&s_splash, 0, sizeof(s_splash));

    display_unlock();
    ESP_LOGI(TAG_UI, "Boot splash hidden");
}
