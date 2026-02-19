/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "ui/theme/theme_default.h"

static lv_style_t s_style_screen;
static lv_style_t s_style_card;
static lv_style_t s_style_button_off;
static lv_style_t s_style_button_on;
static bool s_initialized = false;

void theme_default_init(void)
{
    if (s_initialized) {
        return;
    }

    lv_style_init(&s_style_screen);
    lv_style_set_bg_color(&s_style_screen, lv_color_hex(APP_UI_COLOR_SCREEN_BG));
    lv_style_set_bg_grad_color(&s_style_screen, lv_color_hex(APP_UI_COLOR_SCREEN_BG_GRAD));
    lv_style_set_bg_grad_dir(&s_style_screen, LV_GRAD_DIR_VER);

    lv_style_init(&s_style_card);
    lv_style_set_bg_color(&s_style_card, lv_color_hex(APP_UI_COLOR_CARD_BG_OFF));
    lv_style_set_bg_opa(&s_style_card, LV_OPA_COVER);
    lv_style_set_radius(&s_style_card, APP_UI_CARD_RADIUS);
    lv_style_set_pad_all(&s_style_card, 16);
#if APP_UI_REWORK_V2
    lv_style_set_border_width(&s_style_card, 1);
    lv_style_set_border_color(&s_style_card, lv_color_hex(APP_UI_COLOR_CARD_BORDER));
    lv_style_set_border_opa(&s_style_card, LV_OPA_70);
#else
    lv_style_set_border_width(&s_style_card, 0);
#endif

    lv_style_init(&s_style_button_off);
    lv_style_set_bg_color(&s_style_button_off, lv_color_hex(APP_UI_COLOR_CARD_BG_OFF));
    lv_style_set_bg_opa(&s_style_button_off, LV_OPA_COVER);
    lv_style_set_radius(&s_style_button_off, APP_UI_CARD_RADIUS);
#if APP_UI_REWORK_V2
    lv_style_set_border_width(&s_style_button_off, 1);
    lv_style_set_border_color(&s_style_button_off, lv_color_hex(APP_UI_COLOR_CARD_BORDER));
    lv_style_set_border_opa(&s_style_button_off, LV_OPA_70);
#else
    lv_style_set_border_width(&s_style_button_off, 0);
#endif
    lv_style_set_text_color(&s_style_button_off, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY));

    lv_style_init(&s_style_button_on);
    lv_style_set_bg_color(&s_style_button_on, lv_color_hex(APP_UI_COLOR_CARD_BG_ON));
    lv_style_set_bg_opa(&s_style_button_on, LV_OPA_COVER);
    lv_style_set_radius(&s_style_button_on, APP_UI_CARD_RADIUS);
#if APP_UI_REWORK_V2
    lv_style_set_border_width(&s_style_button_on, 1);
    lv_style_set_border_color(&s_style_button_on, lv_color_hex(APP_UI_COLOR_CARD_BORDER));
    lv_style_set_border_opa(&s_style_button_on, LV_OPA_70);
#else
    lv_style_set_border_width(&s_style_button_on, 0);
#endif
    lv_style_set_text_color(&s_style_button_on, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY));

    s_initialized = true;
}

void theme_default_style_screen(lv_obj_t *obj)
{
    theme_default_init();
    lv_obj_add_style(obj, &s_style_screen, LV_PART_MAIN);
}

void theme_default_style_card(lv_obj_t *obj)
{
    theme_default_init();
    lv_obj_add_style(obj, &s_style_card, LV_PART_MAIN);
}

void theme_default_style_button(lv_obj_t *obj, bool is_on)
{
    theme_default_init();
    lv_obj_remove_style_all(obj);
    lv_obj_add_style(obj, is_on ? &s_style_button_on : &s_style_button_off, LV_PART_MAIN);
}

lv_color_t theme_default_color_text_primary(void)
{
    return lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY);
}

lv_color_t theme_default_color_text_muted(void)
{
    return lv_color_hex(APP_UI_COLOR_TEXT_MUTED);
}
