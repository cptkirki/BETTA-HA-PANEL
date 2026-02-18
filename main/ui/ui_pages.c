#include "ui/ui_pages.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "app_config.h"
#include "ui/fonts/app_text_fonts.h"
#include "ui/ui_i18n.h"
#include "ui/theme/theme_default.h"

typedef struct {
    char id[APP_MAX_PAGE_ID_LEN];
    char title[APP_MAX_NAME_LEN];
    lv_obj_t *container;
} ui_page_entry_t;

static ui_page_entry_t s_pages[APP_MAX_PAGES];
static uint16_t s_page_count = 0;
static int16_t s_current_index = -1;

static lv_obj_t *s_background = NULL;
static lv_obj_t *s_topbar = NULL;
static lv_obj_t *s_content_box = NULL;
static lv_obj_t *s_date_label = NULL;
static lv_obj_t *s_time_label = NULL;
static lv_obj_t *s_wifi_icon = NULL;
static lv_obj_t *s_api_icon = NULL;
static lv_obj_t *s_nav_bar = NULL;
static lv_obj_t *s_nav_home_button = NULL;
static lv_obj_t *s_nav_home_label = NULL;
static lv_obj_t *s_nav_extra_buttons[APP_MAX_PAGES - 1] = {0};
static lv_obj_t *s_nav_extra_labels[APP_MAX_PAGES - 1] = {0};
static uint16_t s_nav_extra_page_index[APP_MAX_PAGES - 1] = {0};

#define TOPBAR_TIME_FONT APP_FONT_TEXT_34

#define TOPBAR_DATE_FONT APP_FONT_TEXT_22

#if LV_FONT_MONTSERRAT_24
#define TOPBAR_ICON_FONT (&lv_font_montserrat_24)
#elif LV_FONT_MONTSERRAT_20
#define TOPBAR_ICON_FONT (&lv_font_montserrat_20)
#else
#define TOPBAR_ICON_FONT LV_FONT_DEFAULT
#endif

#define NAV_TEXT_FONT APP_FONT_TEXT_16

static void ui_pages_style_nav_button(lv_obj_t *btn, lv_obj_t *label, bool selected, bool is_home)
{
    if (btn == NULL || label == NULL) {
        return;
    }

    const lv_color_t chip_bg = lv_color_hex(APP_UI_COLOR_TOPBAR_CHIP_BG);
    const lv_color_t chip_border = lv_color_hex(APP_UI_COLOR_TOPBAR_CHIP_BORDER);
    lv_obj_set_style_bg_color(btn, chip_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, selected ? LV_OPA_80 : LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(btn, selected ? LV_OPA_COVER : LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, chip_border, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(btn, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (is_home) {
        lv_obj_set_style_text_color(
            label,
            selected ? lv_color_hex(APP_UI_COLOR_NAV_HOME_ACTIVE) : lv_color_hex(APP_UI_COLOR_NAV_HOME_IDLE),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_decor(label, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_text_color(
            label,
            selected ? lv_color_hex(APP_UI_COLOR_NAV_TAB_ACTIVE) : lv_color_hex(APP_UI_COLOR_NAV_TAB_IDLE),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_decor(label, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void ui_pages_apply_tab_style(uint16_t selected_index)
{
    if (s_nav_bar == NULL || s_nav_home_button == NULL || s_nav_home_label == NULL) {
        return;
    }

    const lv_coord_t nav_btn_h = 42;
    const lv_coord_t nav_home_w = 72;
    const lv_coord_t nav_btn_y = 9;
    const lv_coord_t nav_outer_margin = 14;
    const lv_coord_t nav_home_gap = 12;
    const lv_coord_t nav_side_gap = 8;
    const lv_coord_t nav_min_side_btn_w = 64;
    const lv_coord_t nav_home_x = (APP_SCREEN_WIDTH - nav_home_w) / 2;

    lv_obj_set_size(s_nav_home_button, nav_home_w, nav_btn_h);
    lv_obj_set_pos(s_nav_home_button, nav_home_x, nav_btn_y);
    ui_pages_style_nav_button(s_nav_home_button, s_nav_home_label, (selected_index == 0), true);
    lv_obj_clear_flag(s_nav_home_button, LV_OBJ_FLAG_HIDDEN);

    uint16_t extra_count = (s_page_count > 1) ? (uint16_t)(s_page_count - 1U) : 0U;
    uint16_t left_count = (uint16_t)((extra_count + 1U) / 2U);
    uint16_t right_count = (uint16_t)(extra_count / 2U);

    lv_coord_t left_start = nav_outer_margin;
    lv_coord_t left_end = nav_home_x - nav_home_gap;
    lv_coord_t right_start = nav_home_x + nav_home_w + nav_home_gap;
    lv_coord_t right_end = APP_SCREEN_WIDTH - nav_outer_margin;

    lv_coord_t left_region_w = (left_end > left_start) ? (left_end - left_start) : 0;
    lv_coord_t right_region_w = (right_end > right_start) ? (right_end - right_start) : 0;

    lv_coord_t left_btn_w = 0;
    lv_coord_t right_btn_w = 0;
    if (left_count > 0) {
        left_btn_w = (left_region_w - ((lv_coord_t)left_count - 1) * nav_side_gap) / (lv_coord_t)left_count;
        if (left_btn_w < nav_min_side_btn_w) {
            left_btn_w = nav_min_side_btn_w;
        }
    }
    if (right_count > 0) {
        right_btn_w = (right_region_w - ((lv_coord_t)right_count - 1) * nav_side_gap) / (lv_coord_t)right_count;
        if (right_btn_w < nav_min_side_btn_w) {
            right_btn_w = nav_min_side_btn_w;
        }
    }

    uint16_t left_slot = 0;
    uint16_t right_slot = 0;
    uint16_t slot = 0;
    for (uint16_t page_index = 1; page_index < s_page_count && slot < (APP_MAX_PAGES - 1); page_index++, slot++) {
        lv_obj_t *btn = s_nav_extra_buttons[slot];
        lv_obj_t *label = s_nav_extra_labels[slot];
        if (btn == NULL || label == NULL) {
            continue;
        }

        bool place_left = (((page_index - 1U) & 0x1U) == 0U);
        lv_coord_t w = place_left ? left_btn_w : right_btn_w;
        lv_coord_t x = 0;
        if (place_left) {
            uint16_t pos = (uint16_t)((left_count - 1U) - left_slot);
            x = left_start + (lv_coord_t)pos * (w + nav_side_gap);
            left_slot++;
        } else {
            uint16_t pos = right_slot;
            x = right_start + (lv_coord_t)pos * (w + nav_side_gap);
            right_slot++;
        }

        s_nav_extra_page_index[slot] = page_index;
        lv_label_set_text(label, s_pages[page_index].title);
        lv_obj_set_size(btn, w, nav_btn_h);
        lv_obj_set_pos(btn, x, nav_btn_y);
        lv_obj_set_width(label, w - 20);
        lv_obj_center(label);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_HIDDEN);
        ui_pages_style_nav_button(btn, label, (page_index == selected_index), false);
    }

    for (; slot < (APP_MAX_PAGES - 1); slot++) {
        s_nav_extra_page_index[slot] = APP_MAX_PAGES;
        if (s_nav_extra_buttons[slot] != NULL) {
            lv_obj_add_flag(s_nav_extra_buttons[slot], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ui_pages_style_topbar_chip(lv_obj_t *obj)
{
    if (obj == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(obj, lv_color_hex(APP_UI_COLOR_TOPBAR_CHIP_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_hex(APP_UI_COLOR_TOPBAR_CHIP_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_opa(obj, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_left(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(obj, TOPBAR_ICON_FONT, LV_PART_MAIN);
}

static void ui_nav_home_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }
    if (s_page_count == 0) {
        return;
    }
    ui_pages_show_index(0);
}

static void ui_nav_extra_button_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    uintptr_t slot = (uintptr_t)lv_event_get_user_data(event);
    if (slot >= (APP_MAX_PAGES - 1)) {
        return;
    }

    uint16_t page_index = s_nav_extra_page_index[slot];
    if (page_index >= s_page_count) {
        return;
    }
    ui_pages_show_index(page_index);
}

static void ui_pages_create_topbar(lv_obj_t *screen)
{
    const lv_coord_t topbar_h = APP_CONTENT_BOX_Y;

    s_topbar = lv_obj_create(screen);
    lv_obj_remove_style_all(s_topbar);
    lv_obj_set_size(s_topbar, APP_SCREEN_WIDTH, topbar_h);
    lv_obj_set_pos(s_topbar, 0, 0);
    lv_obj_clear_flag(s_topbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_topbar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_topbar, lv_color_hex(APP_UI_COLOR_TOPBAR_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_topbar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_topbar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(s_topbar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_topbar, lv_color_hex(APP_UI_COLOR_TOPBAR_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_opa(s_topbar, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_topbar, 0, LV_PART_MAIN);

    s_date_label = lv_label_create(s_topbar);
    lv_obj_set_width(s_date_label, 220);
    lv_obj_align(s_date_label, LV_ALIGN_LEFT_MID, 16, 0);
    lv_obj_set_style_text_color(s_date_label, lv_color_hex(APP_UI_COLOR_TOPBAR_MUTED), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_date_label, TOPBAR_DATE_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_date_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_text(s_date_label, "--.--.----");

    s_time_label = lv_label_create(s_topbar);
    lv_obj_set_width(s_time_label, 220);
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(s_time_label, lv_color_hex(APP_UI_COLOR_TOPBAR_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_time_label, TOPBAR_TIME_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_time_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(s_time_label, "--:--");

    s_api_icon = lv_label_create(s_topbar);
    lv_obj_set_width(s_api_icon, 86);
    lv_obj_align(s_api_icon, LV_ALIGN_RIGHT_MID, -114, 0);
    ui_pages_style_topbar_chip(s_api_icon);
    char api_text[32] = {0};
    snprintf(api_text, sizeof(api_text), "%s %s", ui_i18n_get("topbar.ha", "HA"), LV_SYMBOL_CLOSE);
    lv_label_set_text(s_api_icon, api_text);

    s_wifi_icon = lv_label_create(s_topbar);
    lv_obj_set_width(s_wifi_icon, 96);
    lv_obj_align(s_wifi_icon, LV_ALIGN_RIGHT_MID, -12, 0);
    ui_pages_style_topbar_chip(s_wifi_icon);
    lv_label_set_text(s_wifi_icon, LV_SYMBOL_CLOSE);
}

static void ui_pages_create_nav(lv_obj_t *screen)
{
    s_nav_bar = lv_obj_create(screen);
    lv_obj_remove_style_all(s_nav_bar);
    lv_obj_set_size(s_nav_bar, APP_SCREEN_WIDTH, 60);
    lv_obj_set_pos(s_nav_bar, 0, 660);
    lv_obj_clear_flag(s_nav_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_nav_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_nav_bar, lv_color_hex(APP_UI_COLOR_TOPBAR_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_nav_bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_nav_bar, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(s_nav_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(s_nav_bar, lv_color_hex(APP_UI_COLOR_TOPBAR_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(s_nav_bar, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_nav_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(s_nav_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_nav_home_button = lv_obj_create(s_nav_bar);
    lv_obj_remove_style_all(s_nav_home_button);
    lv_obj_set_ext_click_area(s_nav_home_button, 14);
    lv_obj_add_flag(s_nav_home_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_nav_home_button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_nav_home_button, ui_nav_home_button_event_cb, LV_EVENT_CLICKED, NULL);

    s_nav_home_label = lv_label_create(s_nav_home_button);
    lv_label_set_text(s_nav_home_label, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(s_nav_home_label, TOPBAR_ICON_FONT, LV_PART_MAIN);
    lv_obj_center(s_nav_home_label);

    for (uint16_t i = 0; i < (APP_MAX_PAGES - 1); i++) {
        lv_obj_t *btn = lv_obj_create(s_nav_bar);
        lv_obj_remove_style_all(btn);
        lv_obj_set_ext_click_area(btn, 10);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn, ui_nav_extra_button_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, "");
        lv_obj_set_style_text_font(label, NAV_TEXT_FONT, LV_PART_MAIN);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_center(label);

        s_nav_extra_buttons[i] = btn;
        s_nav_extra_labels[i] = label;
        s_nav_extra_page_index[i] = APP_MAX_PAGES;
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_pages_init(void)
{
    memset(s_pages, 0, sizeof(s_pages));
    s_page_count = 0;
    s_current_index = -1;
    s_background = NULL;
    s_topbar = NULL;
    s_content_box = NULL;
    s_date_label = NULL;
    s_time_label = NULL;
    s_wifi_icon = NULL;
    s_api_icon = NULL;
    s_nav_bar = NULL;
    s_nav_home_button = NULL;
    s_nav_home_label = NULL;
    memset(s_nav_extra_buttons, 0, sizeof(s_nav_extra_buttons));
    memset(s_nav_extra_labels, 0, sizeof(s_nav_extra_labels));
    memset(s_nav_extra_page_index, 0, sizeof(s_nav_extra_page_index));

    lv_obj_t *screen = lv_scr_act();
    lv_obj_clean(screen);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(APP_UI_COLOR_SCREEN_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    s_background = lv_obj_create(screen);
    lv_obj_remove_style_all(s_background);
    lv_obj_set_size(s_background, APP_SCREEN_WIDTH, APP_SCREEN_HEIGHT);
    lv_obj_set_pos(s_background, 0, 0);
    lv_obj_clear_flag(s_background, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_background, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_background, lv_color_hex(APP_UI_COLOR_SCREEN_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_background, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_background, 0, LV_PART_MAIN);

    s_content_box = lv_obj_create(screen);
    lv_obj_remove_style_all(s_content_box);
    lv_obj_set_size(s_content_box, APP_CONTENT_BOX_WIDTH, APP_CONTENT_BOX_HEIGHT);
    lv_obj_set_pos(s_content_box, APP_CONTENT_BOX_X, APP_CONTENT_BOX_Y);
    lv_obj_clear_flag(s_content_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_content_box, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_content_box, lv_color_hex(APP_UI_COLOR_CONTENT_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_content_box, LV_OPA_COVER, LV_PART_MAIN);
#if APP_UI_REWORK_V2
    lv_obj_set_style_border_width(s_content_box, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_content_box, lv_color_hex(APP_UI_COLOR_CONTENT_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_opa(s_content_box, LV_OPA_70, LV_PART_MAIN);
#else
    lv_obj_set_style_border_width(s_content_box, 0, LV_PART_MAIN);
#endif
    lv_obj_set_style_pad_all(s_content_box, 0, LV_PART_MAIN);

    ui_pages_create_topbar(screen);
    ui_pages_create_nav(screen);

    time_t now = time(NULL);
    struct tm info = {0};
    localtime_r(&now, &info);
    ui_pages_set_topbar_datetime(&info);
    ui_pages_set_topbar_status(false, false, false, false);
    ui_pages_apply_tab_style(0);
}

void ui_pages_reset(void)
{
    ui_pages_init();
}

lv_obj_t *ui_pages_add(const char *page_id, const char *title)
{
    if (s_page_count >= APP_MAX_PAGES || page_id == NULL || page_id[0] == '\0' || s_content_box == NULL) {
        return NULL;
    }

    uint16_t index = s_page_count;
    snprintf(s_pages[index].id, sizeof(s_pages[index].id), "%s", page_id);
    snprintf(s_pages[index].title, sizeof(s_pages[index].title), "%s", (title && title[0]) ? title : page_id);

    lv_obj_t *container = lv_obj_create(s_content_box);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, APP_CONTENT_BOX_WIDTH, APP_CONTENT_BOX_HEIGHT);
    lv_obj_set_pos(container, 0, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    s_pages[index].container = container;

    s_page_count++;
    ui_pages_apply_tab_style((uint16_t)(s_current_index >= 0 ? s_current_index : 0));
    return container;
}

bool ui_pages_show_index(uint16_t index)
{
    if (index >= s_page_count) {
        return false;
    }

    for (uint16_t i = 0; i < s_page_count; i++) {
        if (s_pages[i].container == NULL) {
            continue;
        }
        if (i == index) {
            lv_obj_clear_flag(s_pages[i].container, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_pages[i].container, LV_OBJ_FLAG_HIDDEN);
        }
    }
    s_current_index = (int16_t)index;
    ui_pages_apply_tab_style(index);
    return true;
}

bool ui_pages_show(const char *page_id)
{
    if (page_id == NULL) {
        return false;
    }
    for (uint16_t i = 0; i < s_page_count; i++) {
        if (strncmp(page_id, s_pages[i].id, APP_MAX_PAGE_ID_LEN) == 0) {
            return ui_pages_show_index(i);
        }
    }
    return false;
}

bool ui_pages_next(void)
{
    if (s_page_count == 0) {
        return false;
    }
    uint16_t next = (uint16_t)(((s_current_index < 0 ? 0 : s_current_index) + 1) % s_page_count);
    return ui_pages_show_index(next);
}

const char *ui_pages_current_id(void)
{
    if (s_current_index < 0 || (uint16_t)s_current_index >= s_page_count) {
        return "";
    }
    return s_pages[s_current_index].id;
}

uint16_t ui_pages_count(void)
{
    return s_page_count;
}

void ui_pages_set_topbar_status(
    bool wifi_connected, bool wifi_setup_ap_active, bool api_connected, bool api_initial_sync_done)
{
    lv_color_t on = lv_color_hex(APP_UI_COLOR_TOPBAR_STATUS_ON);
    lv_color_t off = lv_color_hex(APP_UI_COLOR_TOPBAR_STATUS_OFF);

    if (s_wifi_icon != NULL) {
        char wifi_text[32] = {0};
        snprintf(wifi_text, sizeof(wifi_text), "%s", LV_SYMBOL_CLOSE);
        lv_color_t wifi_color = off;
        if (wifi_setup_ap_active) {
            snprintf(wifi_text, sizeof(wifi_text), "%s %s", ui_i18n_get("topbar.ap", "AP"), LV_SYMBOL_WIFI);
            wifi_color = on;
        } else if (wifi_connected) {
            snprintf(wifi_text, sizeof(wifi_text), "%s", LV_SYMBOL_WIFI);
            wifi_color = on;
        }
        lv_label_set_text(s_wifi_icon, wifi_text);
        lv_obj_set_style_text_color(s_wifi_icon, wifi_color, LV_PART_MAIN);
    }

    if (s_api_icon != NULL) {
        char api_text[32] = {0};
        snprintf(api_text, sizeof(api_text), "%s %s", ui_i18n_get("topbar.ha", "HA"), LV_SYMBOL_CLOSE);
        lv_color_t api_color = off;
        if (api_connected) {
            if (api_initial_sync_done) {
                snprintf(api_text, sizeof(api_text), "%s %s", ui_i18n_get("topbar.ha", "HA"), LV_SYMBOL_OK);
            } else {
                snprintf(api_text, sizeof(api_text), "%s %s", ui_i18n_get("topbar.ha", "HA"), LV_SYMBOL_REFRESH);
            }
            api_color = on;
        }
        lv_label_set_text(s_api_icon, api_text);
        lv_obj_set_style_text_color(s_api_icon, api_color, LV_PART_MAIN);
    }
}

void ui_pages_set_topbar_datetime(const struct tm *timeinfo)
{
    if (timeinfo == NULL) {
        return;
    }

    char date_buf[32] = {0};
    char time_buf[16] = {0};
    snprintf(
        date_buf, sizeof(date_buf), "%02d.%02d.%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);

    if (s_date_label != NULL) {
        lv_label_set_text(s_date_label, date_buf);
    }
    if (s_time_label != NULL) {
        lv_label_set_text(s_time_label, time_buf);
    }
}
