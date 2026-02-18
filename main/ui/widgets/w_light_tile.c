#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"

#include "ui/fonts/app_text_fonts.h"
#include "ui/ui_bindings.h"
#include "ui/fonts/mdi_font_registry.h"
#include "ui/ui_i18n.h"
#include "ui/theme/theme_default.h"

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    bool is_on;
    int brightness;
    bool unavailable;
    lv_coord_t configured_min_dim;
} w_light_tile_ctx_t;

#define ICON_CP_MDI_LIGHTBULB_ON 0xF06E8U
static const char *TAG = "w_light_tile";

typedef enum {
    LIGHT_TILE_CLASS_COMPACT = 0,
    LIGHT_TILE_CLASS_S,
    LIGHT_TILE_CLASS_M,
    LIGHT_TILE_CLASS_L,
} light_tile_class_t;

typedef struct {
    lv_coord_t card_pad;
    lv_coord_t title_bottom;
    lv_coord_t top_label_y;
    lv_coord_t slider_side_margin;
    lv_coord_t slider_height;
    lv_coord_t slider_bottom;
    lv_coord_t icon_top;
    lv_coord_t icon_gap;
    lv_coord_t icon_bias_y;
    const lv_font_t *title_font;
    const lv_font_t *top_font;
} light_tile_layout_t;

typedef struct {
    lv_obj_t *icon;
    lv_obj_t *title;
    lv_obj_t *state_label;
    lv_obj_t *slider;
    lv_obj_t *value_label;
} light_tile_widgets_t;

static const light_tile_layout_t LIGHT_LAYOUT_COMPACT = {
    .card_pad = 12,
    .title_bottom = -40,
    .top_label_y = 0,
    .slider_side_margin = 14,
    .slider_height = 12,
    .slider_bottom = -10,
    .icon_top = 10,
    .icon_gap = 6,
    .icon_bias_y = 0,
    .title_font = APP_FONT_TEXT_16,
    .top_font = APP_FONT_TEXT_16,
};

static const light_tile_layout_t LIGHT_LAYOUT_S = {
    .card_pad = 14,
    .title_bottom = -46,
    .top_label_y = 2,
    .slider_side_margin = 18,
    .slider_height = 13,
    .slider_bottom = -12,
    .icon_top = 9,
    .icon_gap = 8,
    .icon_bias_y = 2,
    .title_font = APP_FONT_TEXT_16,
    .top_font = APP_FONT_TEXT_16,
};

static const light_tile_layout_t LIGHT_LAYOUT_M = {
    .card_pad = 16,
    .title_bottom = -54,
    .top_label_y = 2,
    .slider_side_margin = 22,
    .slider_height = 15,
    .slider_bottom = -16,
    .icon_top = 8,
    .icon_gap = 10,
    .icon_bias_y = 8,
    .title_font = APP_FONT_TEXT_18,
    .top_font = APP_FONT_TEXT_16,
};

static const light_tile_layout_t LIGHT_LAYOUT_L = {
    .card_pad = 18,
    .title_bottom = -62,
    .top_label_y = 2,
    .slider_side_margin = 26,
    .slider_height = 16,
    .slider_bottom = -18,
    .icon_top = 8,
    .icon_gap = 14,
    .icon_bias_y = 12,
    .title_font = APP_FONT_TEXT_18,
    .top_font = APP_FONT_TEXT_16,
};

static bool light_font_has_icon(const lv_font_t *font)
{
    if (font == NULL) {
        return false;
    }

    lv_font_glyph_dsc_t dsc = {0};
    return lv_font_get_glyph_dsc(font, &dsc, ICON_CP_MDI_LIGHTBULB_ON, 0);
}

static int clamp_percent(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

static bool light_state_is_on(const char *state)
{
    if (state == NULL) {
        return false;
    }
    return strcmp(state, "on") == 0;
}

static const char *light_translate_status(const char *status_text)
{
    if (status_text == NULL || status_text[0] == '\0') {
        return ui_i18n_get("common.off", "OFF");
    }
    if (strcmp(status_text, "ON") == 0 || strcmp(status_text, "on") == 0) {
        return ui_i18n_get("common.on", "ON");
    }
    if (strcmp(status_text, "OFF") == 0 || strcmp(status_text, "off") == 0) {
        return ui_i18n_get("common.off", "OFF");
    }
    if (strcmp(status_text, "unavailable") == 0) {
        return ui_i18n_get("common.unavailable", "unavailable");
    }
    return status_text;
}

static void light_set_value_label(lv_obj_t *label, int value)
{
    if (label == NULL) {
        return;
    }
    char text[16] = {0};
    snprintf(text, sizeof(text), "%d %%", clamp_percent(value));
    lv_label_set_text(label, text);
}

static int light_extract_brightness_percent(const ha_state_t *state, bool is_on)
{
    int value = -1;
    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *brightness_pct = cJSON_GetObjectItemCaseSensitive(attrs, "brightness_pct");
        cJSON *brightness = cJSON_GetObjectItemCaseSensitive(attrs, "brightness");
        if (cJSON_IsNumber(brightness_pct)) {
            value = (int)brightness_pct->valuedouble;
        } else if (cJSON_IsNumber(brightness)) {
            int raw_255 = (int)brightness->valuedouble;
            if (raw_255 < 0) {
                raw_255 = 0;
            }
            if (raw_255 > 255) {
                raw_255 = 255;
            }
            value = (raw_255 * 100 + 127) / 255;
        }
        cJSON_Delete(attrs);
    }
    if (value < 0) {
        value = is_on ? 100 : 0;
    }
    return clamp_percent(value);
}

static const char *light_icon_utf8_from_codepoint(uint32_t codepoint)
{
    static char utf8[5] = {0};

    if (codepoint <= 0x7FU) {
        utf8[0] = (char)codepoint;
        utf8[1] = '\0';
        return utf8;
    }
    if (codepoint <= 0x7FFU) {
        utf8[0] = (char)(0xC0U | ((codepoint >> 6) & 0x1FU));
        utf8[1] = (char)(0x80U | (codepoint & 0x3FU));
        utf8[2] = '\0';
        return utf8;
    }
    if (codepoint <= 0xFFFFU) {
        utf8[0] = (char)(0xE0U | ((codepoint >> 12) & 0x0FU));
        utf8[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        utf8[2] = (char)(0x80U | (codepoint & 0x3FU));
        utf8[3] = '\0';
        return utf8;
    }

    utf8[0] = (char)(0xF0U | ((codepoint >> 18) & 0x07U));
    utf8[1] = (char)(0x80U | ((codepoint >> 12) & 0x3FU));
    utf8[2] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
    utf8[3] = (char)(0x80U | (codepoint & 0x3FU));
    utf8[4] = '\0';
    return utf8;
}

static const lv_font_t *light_icon_font_for_min_dim(lv_coord_t min_dim)
{
    const lv_font_t *font = NULL;
    if (min_dim >= 300) {
        font = mdi_font_icon_72();
    } else if (min_dim >= 240) {
        font = mdi_font_icon_56();
    } else {
        font = mdi_font_icon_42();
    }

    if (!light_font_has_icon(font)) {
        font = mdi_font_large();
    }
    if (!light_font_has_icon(font)) {
        font = mdi_font_weather();
    }
    if (light_font_has_icon(font)) {
        return font;
    }
    return LV_FONT_DEFAULT;
}

static const char *light_icon_text_for_font(const lv_font_t *font)
{
    static bool warned = false;
    if (light_font_has_icon(font)) {
        return light_icon_utf8_from_codepoint(ICON_CP_MDI_LIGHTBULB_ON);
    }
    if (!warned) {
        warned = true;
        ESP_LOGW(TAG, "MDI icon glyph U+%05" PRIX32 " not available, using LV_SYMBOL_POWER fallback", ICON_CP_MDI_LIGHTBULB_ON);
    }
    return LV_SYMBOL_POWER;
}

static lv_coord_t light_card_width(lv_obj_t *card)
{
    if (card == NULL) {
        return 0;
    }
    lv_obj_update_layout(card);
    lv_coord_t width = lv_obj_get_width(card);
    if (width <= 0) {
        width = lv_obj_get_style_width(card, LV_PART_MAIN);
    }
    return (width > 0) ? width : 0;
}

static lv_coord_t light_card_min_dim(lv_obj_t *card)
{
    if (card == NULL) {
        return 0;
    }
    lv_obj_update_layout(card);
    lv_coord_t w = lv_obj_get_width(card);
    lv_coord_t h = lv_obj_get_height(card);

    if (w <= 0) {
        w = lv_obj_get_style_width(card, LV_PART_MAIN);
    }
    if (h <= 0) {
        h = lv_obj_get_style_height(card, LV_PART_MAIN);
    }
    if (w <= 0 || h <= 0) {
        return 0;
    }
    return (w < h) ? w : h;
}

static lv_coord_t light_effective_min_dim(lv_obj_t *card, const w_light_tile_ctx_t *ctx)
{
    lv_coord_t min_dim = light_card_min_dim(card);
    if (min_dim > 0) {
        return min_dim;
    }
    if (ctx != NULL && ctx->configured_min_dim > 0) {
        return ctx->configured_min_dim;
    }
    return 0;
}

static bool light_get_widgets(lv_obj_t *card, light_tile_widgets_t *out)
{
    if (card == NULL || out == NULL) {
        return false;
    }
    out->icon = lv_obj_get_child(card, 0);
    out->title = lv_obj_get_child(card, 1);
    out->state_label = lv_obj_get_child(card, 2);
    out->slider = lv_obj_get_child(card, 3);
    out->value_label = lv_obj_get_child(card, 4);
    return out->icon != NULL && out->title != NULL && out->state_label != NULL && out->slider != NULL && out->value_label != NULL;
}

static light_tile_class_t light_tile_class_from_dim(lv_coord_t min_dim)
{
    if (min_dim <= 0) {
        return LIGHT_TILE_CLASS_S;
    }
    if (min_dim <= 180) {
        return LIGHT_TILE_CLASS_COMPACT;
    }
    if (min_dim < 240) {
        return LIGHT_TILE_CLASS_S;
    }
    if (min_dim < 300) {
        return LIGHT_TILE_CLASS_M;
    }
    return LIGHT_TILE_CLASS_L;
}

static const light_tile_layout_t *light_pick_layout(lv_obj_t *card, const w_light_tile_ctx_t *ctx)
{
    switch (light_tile_class_from_dim(light_effective_min_dim(card, ctx))) {
        case LIGHT_TILE_CLASS_COMPACT:
            return &LIGHT_LAYOUT_COMPACT;
        case LIGHT_TILE_CLASS_M:
            return &LIGHT_LAYOUT_M;
        case LIGHT_TILE_CLASS_L:
            return &LIGHT_LAYOUT_L;
        case LIGHT_TILE_CLASS_S:
        default:
            return &LIGHT_LAYOUT_S;
    }
}

static void light_apply_layout(lv_obj_t *card, const light_tile_layout_t *layout)
{
    if (card == NULL || layout == NULL) {
        return;
    }

    light_tile_widgets_t w = {0};
    if (!light_get_widgets(card, &w)) {
        return;
    }

    lv_coord_t card_w = light_card_width(card);
    if (card_w <= 0) {
        return;
    }
    lv_obj_set_style_pad_all(card, layout->card_pad, LV_PART_MAIN);

    lv_coord_t content_w = card_w - (layout->card_pad * 2);
    if (content_w < 40) {
        content_w = 40;
    }

    lv_coord_t slider_w = card_w - (layout->slider_side_margin * 2);
    if (slider_w < 60) {
        slider_w = 60;
    }

    lv_obj_set_width(w.icon, content_w);
    lv_obj_set_width(w.title, content_w);
    lv_obj_set_width(w.slider, slider_w);
    lv_obj_set_height(w.slider, layout->slider_height);

    lv_obj_set_style_text_font(w.title, layout->title_font, LV_PART_MAIN);
    lv_obj_set_style_text_font(w.state_label, layout->top_font, LV_PART_MAIN);
    lv_obj_set_style_text_font(w.value_label, layout->top_font, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_x(w.icon, 0, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(w.icon, 0, LV_PART_MAIN);
    lv_obj_set_style_transform_zoom(w.icon, 256, LV_PART_MAIN);

    lv_obj_align(w.icon, LV_ALIGN_TOP_MID, 0, layout->icon_top);
    lv_obj_align(w.title, LV_ALIGN_BOTTOM_MID, 0, layout->title_bottom);
    lv_obj_align(w.state_label, LV_ALIGN_TOP_LEFT, 0, layout->top_label_y);
    lv_obj_align(w.value_label, LV_ALIGN_TOP_RIGHT, 0, layout->top_label_y);
    lv_obj_align(w.slider, LV_ALIGN_BOTTOM_MID, 0, layout->slider_bottom);
}

static void light_position_icon_between_state_and_title(lv_obj_t *card, lv_coord_t gap, lv_coord_t bias_y)
{
    if (card == NULL) {
        return;
    }

    lv_obj_t *icon = lv_obj_get_child(card, 0);
    lv_obj_t *title = lv_obj_get_child(card, 1);
    lv_obj_t *state_label = lv_obj_get_child(card, 2);
    if (icon == NULL || title == NULL || state_label == NULL) {
        return;
    }

    /* Layout once so child coordinates/heights are valid before calculating placement. */
    lv_obj_update_layout(card);

    if (gap < 0) {
        gap = 0;
    }
    lv_coord_t top = lv_obj_get_y(state_label) + lv_obj_get_height(state_label) + gap;
    lv_coord_t bottom = lv_obj_get_y(title) - gap;
    lv_coord_t icon_h = lv_obj_get_height(icon);

    if (icon_h < 1) {
        const lv_font_t *font = lv_obj_get_style_text_font(icon, LV_PART_MAIN);
        if (font != NULL) {
            icon_h = font->line_height;
        }
    }

    lv_coord_t y = top;
    lv_coord_t room = bottom - top;
    if (room >= icon_h) {
        y = top + (room - icon_h) / 2;
    }

    lv_coord_t max_y = bottom - icon_h;
    if (max_y < top) {
        max_y = top;
    }
    y += bias_y;
    if (y < top) {
        y = top;
    }
    if (y > max_y) {
        y = max_y;
    }

    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, y);
}

static void light_apply_visual(lv_obj_t *card, const w_light_tile_ctx_t *ctx, bool is_on, int brightness, const char *status_text)
{
    if (card == NULL) {
        return;
    }
    light_tile_widgets_t w = {0};
    if (!light_get_widgets(card, &w)) {
        return;
    }
    const light_tile_layout_t *layout = light_pick_layout(card, ctx);
    light_apply_layout(card, layout);
    const lv_coord_t min_dim = light_effective_min_dim(card, ctx);
    const lv_font_t *icon_font = light_icon_font_for_min_dim(min_dim);

    lv_obj_set_style_bg_color(
        card, is_on ? lv_color_hex(APP_UI_COLOR_CARD_BG_ON) : lv_color_hex(APP_UI_COLOR_CARD_BG_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(w.icon, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_ICON_ON) : lv_color_hex(APP_UI_COLOR_CARD_ICON_OFF), LV_PART_MAIN);
    lv_obj_set_style_text_color(w.title, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(
        w.state_label, is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF), LV_PART_MAIN);

    lv_obj_set_style_bg_color(
        w.slider, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_TRACK_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_TRACK_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(w.slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        w.slider, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_IND_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_IND_OFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(w.slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(
        w.slider, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_KNOB_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_KNOB_OFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(w.slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_text_color(
        w.value_label, is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF), LV_PART_MAIN);

    lv_obj_set_style_text_font(w.icon, icon_font, LV_PART_MAIN);
    lv_slider_set_value(w.slider, clamp_percent(brightness), LV_ANIM_OFF);
    light_set_value_label(w.value_label, brightness);
    lv_label_set_text(w.icon, light_icon_text_for_font(icon_font));
    lv_label_set_text(w.state_label, light_translate_status(status_text != NULL ? status_text : (is_on ? "ON" : "OFF")));
    light_position_icon_between_state_and_title(card, layout->icon_gap, layout->icon_bias_y);
}

static void w_light_tile_card_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    w_light_tile_ctx_t *ctx = (w_light_tile_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        if (ctx->unavailable) {
            return;
        }

        lv_obj_t *card = lv_event_get_target(event);
        bool prev_is_on = ctx->is_on;
        int prev_brightness = ctx->brightness;
        bool prev_unavailable = ctx->unavailable;

        bool next_is_on = !ctx->is_on;
        int next_brightness = ctx->brightness;
        if (next_is_on && next_brightness <= 0) {
            next_brightness = 100;
        }
        if (next_brightness < 0) {
            next_brightness = 0;
        }
        next_brightness = clamp_percent(next_brightness);

        esp_err_t err = ui_bindings_set_entity_power(ctx->entity_id, next_is_on);
        if (err == ESP_OK) {
            ctx->is_on = next_is_on;
            ctx->brightness = next_brightness;
            ctx->unavailable = false;
            light_apply_visual(card, ctx, ctx->is_on, ctx->brightness, ctx->is_on ? "ON" : "OFF");
        } else {
            ctx->is_on = prev_is_on;
            ctx->brightness = prev_brightness;
            ctx->unavailable = prev_unavailable;
            light_apply_visual(card, ctx, ctx->is_on, ctx->brightness, ctx->unavailable ? "unavailable" :
                (ctx->is_on ? "ON" : "OFF"));
        }
    } else if (code == LV_EVENT_SIZE_CHANGED) {
        lv_obj_t *card = lv_event_get_target(event);
        light_apply_visual(card, ctx, ctx->is_on, ctx->brightness, ctx->unavailable ? "unavailable" : (ctx->is_on ? "ON" : "OFF"));
    } else if (code == LV_EVENT_DELETE) {
        free(ctx);
    }
}

static void w_light_tile_slider_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) {
        return;
    }

    w_light_tile_ctx_t *ctx = (w_light_tile_ctx_t *)lv_event_get_user_data(event);
    lv_obj_t *slider = lv_event_get_target(event);
    lv_obj_t *card = (slider != NULL) ? lv_obj_get_parent(slider) : NULL;
    lv_obj_t *value_label = (card != NULL) ? lv_obj_get_child(card, 4) : NULL;
    int value = (slider != NULL) ? lv_slider_get_value(slider) : 0;

    if (code == LV_EVENT_VALUE_CHANGED) {
        if (ctx != NULL) {
            ctx->brightness = clamp_percent(value);
        }
        light_set_value_label(value_label, value);
        return;
    }

    if (ctx != NULL) {
        bool prev_is_on = ctx->is_on;
        int prev_brightness = ctx->brightness;
        bool prev_unavailable = ctx->unavailable;

        int next_brightness = clamp_percent(value);
        bool next_is_on = (next_brightness > 0);

        esp_err_t err = ui_bindings_set_slider_value(ctx->entity_id, next_brightness);
        if (err == ESP_OK) {
            ctx->brightness = next_brightness;
            ctx->is_on = next_is_on;
            ctx->unavailable = false;
            if (card != NULL) {
                light_apply_visual(card, ctx, ctx->is_on, ctx->brightness, ctx->is_on ? "ON" : "OFF");
            }
        } else {
            ctx->is_on = prev_is_on;
            ctx->brightness = prev_brightness;
            ctx->unavailable = prev_unavailable;
            if (card != NULL) {
                light_apply_visual(card, ctx, ctx->is_on, ctx->brightness, ctx->unavailable ? "unavailable" :
                    (ctx->is_on ? "ON" : "OFF"));
            }
        }
    }
}

esp_err_t w_light_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
{
    if (def == NULL || parent == NULL || out_instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, def->x, def->y);
    lv_obj_set_size(card, def->w, def->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(card, APP_UI_CARD_RADIUS, LV_PART_MAIN);
#if APP_UI_REWORK_V2
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(APP_UI_COLOR_CARD_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_opa(card, LV_OPA_70, LV_PART_MAIN);
#else
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
#endif
    lv_obj_set_style_pad_all(card, 14, LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(card);
    lv_coord_t configured_min_dim = (def->w < def->h) ? def->w : def->h;
    const lv_font_t *icon_font = light_icon_font_for_min_dim(configured_min_dim);
    lv_label_set_text(icon, light_icon_text_for_font(icon_font));
    lv_obj_set_width(icon, def->w);
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, icon_font, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->id);
    lv_obj_set_width(title, def->w);
    lv_obj_set_style_text_font(title, APP_FONT_TEXT_16, LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -46);

    lv_obj_t *state_label = lv_label_create(card);
    lv_label_set_text(state_label, ui_i18n_get("common.off", "OFF"));
    lv_obj_set_style_text_font(state_label, APP_FONT_TEXT_16, LV_PART_MAIN);
    lv_obj_align(state_label, LV_ALIGN_TOP_LEFT, 0, 2);

    lv_obj_t *slider = lv_slider_create(card);
    lv_obj_set_width(slider, def->w);
    lv_obj_set_height(slider, 13);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *value_label = lv_label_create(card);
    lv_label_set_text(value_label, "0 %");
    lv_obj_set_style_text_font(value_label, APP_FONT_TEXT_16, LV_PART_MAIN);
    lv_obj_align(value_label, LV_ALIGN_TOP_RIGHT, 0, 2);

    w_light_tile_ctx_t *ctx = calloc(1, sizeof(w_light_tile_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }
    snprintf(ctx->entity_id, sizeof(ctx->entity_id), "%s", def->entity_id);
    ctx->is_on = false;
    ctx->brightness = 0;
    ctx->unavailable = false;
    ctx->configured_min_dim = configured_min_dim;

    lv_obj_add_event_cb(card, w_light_tile_card_event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(card, w_light_tile_card_event_cb, LV_EVENT_SIZE_CHANGED, ctx);
    lv_obj_add_event_cb(card, w_light_tile_card_event_cb, LV_EVENT_DELETE, ctx);
    lv_obj_add_event_cb(slider, w_light_tile_slider_event_cb, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(slider, w_light_tile_slider_event_cb, LV_EVENT_RELEASED, ctx);

    light_apply_visual(card, ctx, false, 0, "OFF");
    out_instance->ctx = ctx;
    out_instance->obj = card;
    return ESP_OK;
}

void w_light_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }
    bool is_on = light_state_is_on(state->state);
    int brightness = light_extract_brightness_percent(state, is_on);
    w_light_tile_ctx_t *ctx = (w_light_tile_ctx_t *)instance->ctx;
    if (ctx != NULL) {
        ctx->is_on = is_on;
        ctx->brightness = brightness;
        ctx->unavailable = false;
    }
    light_apply_visual(instance->obj, ctx, is_on, brightness, is_on ? "ON" : "OFF");
}

void w_light_tile_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    w_light_tile_ctx_t *ctx = (w_light_tile_ctx_t *)instance->ctx;
    if (ctx != NULL) {
        ctx->is_on = false;
        ctx->brightness = 0;
        ctx->unavailable = true;
    }
    light_apply_visual(instance->obj, ctx, false, 0, "unavailable");
}
