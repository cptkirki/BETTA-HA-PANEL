#include "ui/ui_widget_factory.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"

typedef enum {
    W_SLIDER_DIR_AUTO = 0,
    W_SLIDER_DIR_LEFT_TO_RIGHT,
    W_SLIDER_DIR_RIGHT_TO_LEFT,
    W_SLIDER_DIR_BOTTOM_TO_TOP,
    W_SLIDER_DIR_TOP_TO_BOTTOM,
} w_slider_direction_t;

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    lv_obj_t *card;
    lv_obj_t *title_label;
    lv_obj_t *state_label;
    lv_obj_t *value_label;
    lv_obj_t *slider;
    w_slider_direction_t direction_cfg;
    w_slider_direction_t direction_effective;
    lv_color_t accent_color;
    int value;
    bool is_on;
    bool unavailable;
    bool dragging;
    bool suppress_event;
    int last_sent_value;
} w_slider_ctx_t;

static const uint32_t W_SLIDER_FILL_OFF_HEX = 0x8C98A4;
static const uint32_t W_SLIDER_TRACK_HEX = 0x3A3E43;

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

static bool slider_state_is_unavailable(const char *state)
{
    if (state == NULL) {
        return false;
    }
    return strcmp(state, "unavailable") == 0 || strcmp(state, "unknown") == 0;
}

static bool slider_state_is_on_text(const char *state)
{
    if (state == NULL) {
        return false;
    }
    return strcmp(state, "on") == 0 || strcmp(state, "open") == 0 || strcmp(state, "playing") == 0 ||
           strcmp(state, "home") == 0;
}

static bool slider_parse_percent_text(const char *text, int *out_value)
{
    if (text == NULL || out_value == NULL || text[0] == '\0') {
        return false;
    }

    char *end = NULL;
    double parsed = strtod(text, &end);
    if (end == text) {
        return false;
    }

    while (*end != '\0' && isspace((unsigned char)*end)) {
        end++;
    }
    if (*end == '%') {
        end++;
    }
    while (*end != '\0' && isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != '\0') {
        return false;
    }

    int value = (int)(parsed + (parsed >= 0.0 ? 0.5 : -0.5));
    *out_value = clamp_percent(value);
    return true;
}

static int slider_extract_percent_value(const ha_state_t *state, bool *out_has_numeric)
{
    if (out_has_numeric != NULL) {
        *out_has_numeric = false;
    }
    if (state == NULL) {
        return 0;
    }

    int value = 0;
    if (slider_parse_percent_text(state->state, &value)) {
        if (out_has_numeric != NULL) {
            *out_has_numeric = true;
        }
        return value;
    }

    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *brightness_pct = cJSON_GetObjectItemCaseSensitive(attrs, "brightness_pct");
        cJSON *brightness = cJSON_GetObjectItemCaseSensitive(attrs, "brightness");
        cJSON *volume_level = cJSON_GetObjectItemCaseSensitive(attrs, "volume_level");
        if (cJSON_IsNumber(brightness_pct)) {
            value = clamp_percent((int)(brightness_pct->valuedouble + 0.5));
            if (out_has_numeric != NULL) {
                *out_has_numeric = true;
            }
        } else if (cJSON_IsNumber(brightness)) {
            int raw_255 = (int)(brightness->valuedouble + 0.5);
            if (raw_255 < 0) {
                raw_255 = 0;
            }
            if (raw_255 > 255) {
                raw_255 = 255;
            }
            value = (raw_255 * 100 + 127) / 255;
            if (out_has_numeric != NULL) {
                *out_has_numeric = true;
            }
        } else if (cJSON_IsNumber(volume_level)) {
            double normalized = volume_level->valuedouble;
            if (normalized < 0.0) {
                normalized = 0.0;
            }
            if (normalized > 1.0) {
                normalized = 1.0;
            }
            value = clamp_percent((int)(normalized * 100.0 + 0.5));
            if (out_has_numeric != NULL) {
                *out_has_numeric = true;
            }
        }
        cJSON_Delete(attrs);
    }

    if (out_has_numeric != NULL && *out_has_numeric) {
        return clamp_percent(value);
    }
    return slider_state_is_on_text(state->state) ? 100 : 0;
}

static bool slider_is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int slider_hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

static bool slider_parse_hex_color(const char *text, lv_color_t *out)
{
    if (text == NULL || out == NULL || text[0] == '\0') {
        return false;
    }

    const char *p = text;
    if (p[0] == '#') {
        p++;
    } else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }

    if (strlen(p) != 6) {
        return false;
    }
    for (size_t i = 0; i < 6; i++) {
        if (!slider_is_hex_digit(p[i])) {
            return false;
        }
    }

    int r_hi = slider_hex_nibble(p[0]);
    int r_lo = slider_hex_nibble(p[1]);
    int g_hi = slider_hex_nibble(p[2]);
    int g_lo = slider_hex_nibble(p[3]);
    int b_hi = slider_hex_nibble(p[4]);
    int b_lo = slider_hex_nibble(p[5]);
    if (r_hi < 0 || r_lo < 0 || g_hi < 0 || g_lo < 0 || b_hi < 0 || b_lo < 0) {
        return false;
    }

    uint32_t rgb = (uint32_t)(((r_hi << 4) | r_lo) << 16) | (uint32_t)(((g_hi << 4) | g_lo) << 8) |
                   (uint32_t)((b_hi << 4) | b_lo);
    *out = lv_color_hex(rgb);
    return true;
}

static w_slider_direction_t slider_direction_from_text(const char *direction)
{
    if (direction == NULL || direction[0] == '\0' || strcmp(direction, "auto") == 0) {
        return W_SLIDER_DIR_AUTO;
    }
    if (strcmp(direction, "left_to_right") == 0) {
        return W_SLIDER_DIR_LEFT_TO_RIGHT;
    }
    if (strcmp(direction, "right_to_left") == 0) {
        return W_SLIDER_DIR_RIGHT_TO_LEFT;
    }
    if (strcmp(direction, "bottom_to_top") == 0) {
        return W_SLIDER_DIR_BOTTOM_TO_TOP;
    }
    if (strcmp(direction, "top_to_bottom") == 0) {
        return W_SLIDER_DIR_TOP_TO_BOTTOM;
    }
    return W_SLIDER_DIR_AUTO;
}

static bool slider_direction_is_vertical(w_slider_direction_t direction)
{
    return direction == W_SLIDER_DIR_BOTTOM_TO_TOP || direction == W_SLIDER_DIR_TOP_TO_BOTTOM;
}

static bool slider_direction_is_reversed(w_slider_direction_t direction)
{
    return direction == W_SLIDER_DIR_RIGHT_TO_LEFT || direction == W_SLIDER_DIR_TOP_TO_BOTTOM;
}

static w_slider_direction_t slider_effective_direction(const w_slider_ctx_t *ctx, lv_obj_t *card)
{
    if (ctx == NULL) {
        return W_SLIDER_DIR_LEFT_TO_RIGHT;
    }
    if (ctx->direction_cfg != W_SLIDER_DIR_AUTO) {
        return ctx->direction_cfg;
    }
    if (card != NULL && lv_obj_get_width(card) < lv_obj_get_height(card)) {
        return W_SLIDER_DIR_BOTTOM_TO_TOP;
    }
    return W_SLIDER_DIR_LEFT_TO_RIGHT;
}

static void slider_apply_native_orientation(w_slider_ctx_t *ctx)
{
    if (ctx == NULL || ctx->slider == NULL) {
        return;
    }

    const bool vertical = slider_direction_is_vertical(ctx->direction_effective);
    const bool reversed = slider_direction_is_reversed(ctx->direction_effective);

    lv_slider_set_orientation(
        ctx->slider, vertical ? LV_SLIDER_ORIENTATION_VERTICAL : LV_SLIDER_ORIENTATION_HORIZONTAL);
    lv_obj_set_style_base_dir(ctx->slider, LV_BASE_DIR_LTR, LV_PART_MAIN);
    lv_slider_set_range(ctx->slider, reversed ? 100 : 0, reversed ? 0 : 100);
}

static void slider_set_value_label(lv_obj_t *label, int value)
{
    if (label == NULL) {
        return;
    }
    char text[24] = {0};
    snprintf(text, sizeof(text), "%d %%", clamp_percent(value));
    lv_label_set_text(label, text);
}

static void slider_apply_layout(lv_obj_t *card, w_slider_ctx_t *ctx)
{
    if (card == NULL || ctx == NULL || ctx->state_label == NULL || ctx->title_label == NULL || ctx->slider == NULL) {
        return;
    }

    lv_obj_align(ctx->state_label, LV_ALIGN_TOP_LEFT, 0, APP_UI_TILE_LAYOUT_TUNED ? 2 : 0);
    lv_obj_align(ctx->value_label, LV_ALIGN_TOP_RIGHT, 0, APP_UI_TILE_LAYOUT_TUNED ? 2 : 0);
    lv_obj_align(ctx->title_label, LV_ALIGN_BOTTOM_MID, 0, APP_UI_TILE_LAYOUT_TUNED ? -12 : -10);

    lv_obj_update_layout(card);

    const lv_coord_t top_gap = APP_UI_TILE_LAYOUT_TUNED ? 10 : 8;
    const lv_coord_t bottom_gap = APP_UI_TILE_LAYOUT_TUNED ? 12 : 10;
    const lv_coord_t min_h = 50;

    lv_coord_t content_w =
        lv_obj_get_width(card) - lv_obj_get_style_pad_left(card, LV_PART_MAIN) - lv_obj_get_style_pad_right(card, LV_PART_MAIN);
    lv_coord_t content_h =
        lv_obj_get_height(card) - lv_obj_get_style_pad_top(card, LV_PART_MAIN) - lv_obj_get_style_pad_bottom(card, LV_PART_MAIN);
    if (content_w < 24) {
        content_w = 24;
    }
    if (content_h < 24) {
        content_h = 24;
    }

    lv_coord_t top = lv_obj_get_y(ctx->state_label) + lv_obj_get_height(ctx->state_label) + top_gap;
    lv_coord_t bottom = lv_obj_get_y(ctx->title_label) - bottom_gap;
    if (top < 0) {
        top = 0;
    }
    if (bottom > content_h) {
        bottom = content_h;
    }
    if (bottom < (top + min_h)) {
        bottom = top + min_h;
        if (bottom > content_h) {
            bottom = content_h;
            top = bottom - min_h;
            if (top < 0) {
                top = 0;
            }
        }
    }

    lv_coord_t area_h = bottom - top;
    lv_coord_t area_w = content_w;

    ctx->direction_effective = slider_effective_direction(ctx, card);
    bool vertical = slider_direction_is_vertical(ctx->direction_effective);

    lv_coord_t slider_x = 0;
    lv_coord_t slider_y = top;
    lv_coord_t slider_w = area_w;
    lv_coord_t slider_h = area_h;
    lv_coord_t target_thickness = (content_w < content_h) ? content_w : content_h;
    if (target_thickness < 2) {
        target_thickness = 2;
    }

    if (vertical) {
        slider_w = target_thickness;
        if (slider_w > area_w) {
            slider_w = area_w;
        }
        if (slider_w < 2) {
            slider_w = 2;
        }
        slider_h = area_h;
        slider_x = (area_w - slider_w) / 2;
    } else {
        slider_w = area_w;
        slider_h = target_thickness;
        if (slider_h > area_h) {
            slider_h = area_h;
        }
        if (slider_h < 2) {
            slider_h = 2;
        }
        slider_y = top + (area_h - slider_h) / 2;
    }

    lv_obj_set_pos(ctx->slider, slider_x, slider_y);
    lv_obj_set_size(ctx->slider, slider_w, slider_h);
    lv_coord_t thickness = vertical ? slider_w : slider_h;
    if (thickness < 2) {
        thickness = 2;
    }
    lv_coord_t radius = thickness / 2;
    if (radius < 1) {
        radius = 1;
    }
    lv_obj_set_style_radius(ctx->slider, radius, LV_PART_MAIN);
    lv_obj_set_style_radius(ctx->slider, radius, LV_PART_INDICATOR);
    lv_obj_set_style_radius(ctx->slider, radius, LV_PART_KNOB);

    slider_apply_native_orientation(ctx);
}

static void slider_apply_visual(w_slider_ctx_t *ctx)
{
    if (ctx == NULL || ctx->card == NULL || ctx->title_label == NULL || ctx->state_label == NULL ||
        ctx->value_label == NULL || ctx->slider == NULL) {
        return;
    }

    lv_obj_t *card = ctx->card;

    const lv_color_t card_bg = lv_color_hex(ctx->is_on && !ctx->unavailable ? APP_UI_COLOR_CARD_BG_ON : APP_UI_COLOR_CARD_BG_OFF);
    const lv_color_t indicator_color = ctx->unavailable
                                           ? lv_color_hex(APP_UI_COLOR_CARD_BORDER)
                                           : (ctx->is_on ? ctx->accent_color : lv_color_hex(W_SLIDER_FILL_OFF_HEX));
    const lv_color_t value_color = ctx->unavailable
                                       ? lv_color_hex(APP_UI_COLOR_TEXT_MUTED)
                                       : (ctx->is_on ? ctx->accent_color : lv_color_hex(APP_UI_COLOR_STATE_OFF));
    const lv_color_t state_color = ctx->unavailable
                                       ? lv_color_hex(APP_UI_COLOR_TEXT_MUTED)
                                       : (ctx->is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF));

    lv_obj_set_style_bg_color(card, card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->title_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->state_label, state_color, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->value_label, value_color, LV_PART_MAIN);

    lv_obj_set_style_bg_color(ctx->slider, lv_color_hex(W_SLIDER_TRACK_HEX), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ctx->slider, lv_color_hex(W_SLIDER_TRACK_HEX), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ctx->slider, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ctx->slider, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(ctx->slider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ctx->slider, 0, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_pad_all(ctx->slider, 0, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(ctx->slider, true, LV_PART_MAIN);

    lv_obj_set_style_bg_color(ctx->slider, indicator_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ctx->slider, indicator_color, LV_PART_INDICATOR | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ctx->slider, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ctx->slider, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(ctx->slider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ctx->slider, 0, LV_PART_INDICATOR | LV_STATE_PRESSED);

    /* Keep native knob hit-testing but render it transparent in all interaction states. */
    lv_obj_set_style_bg_opa(ctx->slider, LV_OPA_TRANSP, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ctx->slider, LV_OPA_TRANSP, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(ctx->slider, LV_OPA_TRANSP, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ctx->slider, LV_OPA_TRANSP, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(ctx->slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ctx->slider, 0, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(ctx->slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ctx->slider, 0, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ctx->slider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ctx->slider, 0, LV_PART_KNOB | LV_STATE_PRESSED);
    lv_obj_set_style_pad_left(ctx->slider, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_right(ctx->slider, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_top(ctx->slider, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_bottom(ctx->slider, 0, LV_PART_KNOB);

    slider_apply_layout(card, ctx);

    ctx->suppress_event = true;
    lv_slider_set_value(ctx->slider, clamp_percent(ctx->value), LV_ANIM_OFF);
    ctx->suppress_event = false;

    slider_set_value_label(ctx->value_label, ctx->value);
    lv_label_set_text(ctx->state_label, ctx->unavailable ? "unavailable" : (ctx->is_on ? "ON" : "OFF"));
}

static void w_slider_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    w_slider_ctx_t *ctx = (w_slider_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }
    if (code == LV_EVENT_DELETE) {
        free(ctx);
        return;
    }

    if (ctx->suppress_event) {
        return;
    }

    lv_obj_t *slider = lv_event_get_target(event);
    if (slider == NULL) {
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        ctx->dragging = true;
        slider_apply_visual(ctx);
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        ctx->value = clamp_percent(lv_slider_get_value(slider));
        ctx->dragging = true;
        ctx->unavailable = false;
        ctx->is_on = ctx->value > 0;
        slider_apply_visual(ctx);
    } else if (code == LV_EVENT_RELEASED) {
        ctx->value = clamp_percent(lv_slider_get_value(slider));
        ctx->dragging = false;
        ctx->unavailable = false;
        ctx->is_on = ctx->value > 0;
        slider_apply_visual(ctx);
        if (ctx->value != ctx->last_sent_value) {
            esp_err_t err = ui_bindings_set_slider_value(ctx->entity_id, ctx->value);
            if (err == ESP_OK) {
                ctx->last_sent_value = ctx->value;
            }
        }
    } else if (code == LV_EVENT_PRESS_LOST) {
        ctx->dragging = false;
        slider_apply_visual(ctx);
    }
}

esp_err_t w_slider_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
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
    lv_obj_set_style_pad_all(card, 16, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->id);
    lv_obj_set_width(title, def->w - 32);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, APP_UI_TILE_LAYOUT_TUNED ? -12 : -10);

    lv_obj_t *state = lv_label_create(card);
    lv_label_set_text(state, "OFF");
    lv_obj_set_style_text_font(state, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(state, LV_ALIGN_TOP_LEFT, 0, APP_UI_TILE_LAYOUT_TUNED ? 2 : 0);

    lv_obj_t *value = lv_label_create(card);
    slider_set_value_label(value, 0);
    lv_obj_set_style_text_font(value, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_align(value, LV_ALIGN_TOP_RIGHT, 0, APP_UI_TILE_LAYOUT_TUNED ? 2 : 0);

    lv_obj_t *slider = lv_slider_create(card);
    lv_obj_set_size(slider, def->w - 32, def->h - 84);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_EVENT_BUBBLE);

    w_slider_ctx_t *ctx = calloc(1, sizeof(w_slider_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }
    snprintf(ctx->entity_id, sizeof(ctx->entity_id), "%s", def->entity_id);
    ctx->card = card;
    ctx->title_label = title;
    ctx->state_label = state;
    ctx->value_label = value;
    ctx->slider = slider;
    ctx->direction_cfg = slider_direction_from_text(def->slider_direction);
    ctx->direction_effective = slider_effective_direction(ctx, card);
    ctx->accent_color = lv_color_hex(APP_UI_COLOR_NAV_TAB_ACTIVE);
    ctx->value = 0;
    ctx->is_on = false;
    ctx->unavailable = false;
    ctx->dragging = false;
    ctx->suppress_event = false;
    ctx->last_sent_value = -1;

    lv_color_t parsed_color = lv_color_hex(0);
    if (slider_parse_hex_color(def->slider_accent_color, &parsed_color)) {
        ctx->accent_color = parsed_color;
    }

    lv_obj_add_event_cb(slider, w_slider_event_cb, LV_EVENT_PRESSED, ctx);
    lv_obj_add_event_cb(slider, w_slider_event_cb, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(slider, w_slider_event_cb, LV_EVENT_RELEASED, ctx);
    lv_obj_add_event_cb(slider, w_slider_event_cb, LV_EVENT_PRESS_LOST, ctx);
    lv_obj_add_event_cb(slider, w_slider_event_cb, LV_EVENT_DELETE, ctx);

    slider_apply_visual(ctx);
    out_instance->obj = card;
    out_instance->ctx = ctx;
    return ESP_OK;
}

void w_slider_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }

    w_slider_ctx_t *ctx = (w_slider_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }

    if (slider_state_is_unavailable(state->state)) {
        ctx->value = 0;
        ctx->is_on = false;
        ctx->unavailable = true;
        ctx->dragging = false;
        slider_apply_visual(ctx);
        return;
    }

    bool has_numeric = false;
    int value = slider_extract_percent_value(state, &has_numeric);
    bool on_from_text = slider_state_is_on_text(state->state);
    bool is_on = has_numeric ? (value > 0 || on_from_text) : on_from_text;

    ctx->value = clamp_percent(value);
    ctx->is_on = is_on;
    ctx->unavailable = false;
    ctx->dragging = false;
    slider_apply_visual(ctx);
}

void w_slider_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }

    w_slider_ctx_t *ctx = (w_slider_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }

    ctx->value = 0;
    ctx->is_on = false;
    ctx->unavailable = true;
    ctx->dragging = false;
    slider_apply_visual(ctx);
}
