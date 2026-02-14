#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"

#include "ui/ui_bindings.h"
#include "ui/fonts/mdi_font_registry.h"
#include "ui/theme/theme_default.h"

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
} w_light_tile_ctx_t;

#define ICON_CP_MDI_LIGHTBULB_ON 0xF06E8U
static const char *TAG = "w_light_tile";

static bool light_mdi_icon_available(void)
{
    static int cached = -1;
    if (cached >= 0) {
        return cached == 1;
    }

    const lv_font_t *font = mdi_font_large();
    if (font == NULL) {
        cached = 0;
        return false;
    }

    lv_font_glyph_dsc_t dsc = {0};
    cached = lv_font_get_glyph_dsc(font, &dsc, ICON_CP_MDI_LIGHTBULB_ON, 0) ? 1 : 0;
    if (cached == 0) {
        ESP_LOGW(TAG, "MDI icon glyph U+%05" PRIX32 " not available, using LV_SYMBOL_POWER fallback", ICON_CP_MDI_LIGHTBULB_ON);
    }
    return cached == 1;
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

static const lv_font_t *light_icon_font(void)
{
    const lv_font_t *font = mdi_font_large();
    if (light_mdi_icon_available() && font != NULL) {
        return font;
    }
    return LV_FONT_DEFAULT;
}

static const char *light_icon_text(void)
{
    return light_mdi_icon_available() ? light_icon_utf8_from_codepoint(ICON_CP_MDI_LIGHTBULB_ON) : LV_SYMBOL_POWER;
}

static void light_position_icon_between_state_and_title(lv_obj_t *card)
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

    const lv_coord_t gap =
#if APP_UI_TILE_LAYOUT_TUNED
        10;
#else
        8;
#endif
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

    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, y);
}

static void light_apply_visual(lv_obj_t *card, bool is_on, int brightness, const char *status_text)
{
    if (card == NULL) {
        return;
    }
    lv_obj_t *icon = lv_obj_get_child(card, 0);
    lv_obj_t *title = lv_obj_get_child(card, 1);
    lv_obj_t *state_label = lv_obj_get_child(card, 2);
    lv_obj_t *slider = lv_obj_get_child(card, 3);
    lv_obj_t *value_label = lv_obj_get_child(card, 4);
    if (icon == NULL || title == NULL || state_label == NULL || slider == NULL || value_label == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(
        card, is_on ? lv_color_hex(APP_UI_COLOR_CARD_BG_ON) : lv_color_hex(APP_UI_COLOR_CARD_BG_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_ICON_ON) : lv_color_hex(APP_UI_COLOR_CARD_ICON_OFF), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(
        state_label, is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF), LV_PART_MAIN);

    lv_obj_set_style_bg_color(
        slider, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_TRACK_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_TRACK_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        slider, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_IND_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_IND_OFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(
        slider, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_KNOB_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_KNOB_OFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_text_color(
        value_label, is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF), LV_PART_MAIN);

    lv_obj_set_style_text_font(icon, light_icon_font(), LV_PART_MAIN);
    lv_slider_set_value(slider, clamp_percent(brightness), LV_ANIM_OFF);
    light_set_value_label(value_label, brightness);
    lv_label_set_text(icon, light_icon_text());
    lv_label_set_text(state_label, status_text != NULL ? status_text : (is_on ? "ON" : "OFF"));
    light_position_icon_between_state_and_title(card);
}

static void w_light_tile_card_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    w_light_tile_ctx_t *ctx = (w_light_tile_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        ui_bindings_toggle_entity(ctx->entity_id);
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
        light_set_value_label(value_label, value);
        return;
    }

    if (ctx != NULL) {
        ui_bindings_set_slider_value(ctx->entity_id, value);
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
    lv_obj_set_style_pad_all(card, 16, LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(card);
    lv_label_set_text(icon, light_icon_text());
    lv_obj_set_width(icon, def->w - 32);
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, light_icon_font(), LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 8);
#else
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);
#endif

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->id);
    lv_obj_set_width(title, def->w - 32);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -54);
#else
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -48);
#endif

    lv_obj_t *state_label = lv_label_create(card);
    lv_label_set_text(state_label, "OFF");
    lv_obj_set_style_text_font(state_label, LV_FONT_DEFAULT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(state_label, LV_ALIGN_TOP_LEFT, 0, 2);
#else
    lv_obj_align(state_label, LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    lv_obj_t *slider = lv_slider_create(card);
    lv_obj_set_width(slider, def->w - 48);
    lv_obj_set_height(slider, 15);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_clear_flag(slider, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *value_label = lv_label_create(card);
    lv_label_set_text(value_label, "0 %");
    lv_obj_set_style_text_font(value_label, LV_FONT_DEFAULT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(value_label, LV_ALIGN_TOP_RIGHT, 0, 2);
#else
    lv_obj_align(value_label, LV_ALIGN_TOP_RIGHT, 0, 0);
#endif

    w_light_tile_ctx_t *ctx = calloc(1, sizeof(w_light_tile_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }
    snprintf(ctx->entity_id, sizeof(ctx->entity_id), "%s", def->entity_id);

    lv_obj_add_event_cb(card, w_light_tile_card_event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(card, w_light_tile_card_event_cb, LV_EVENT_DELETE, ctx);
    lv_obj_add_event_cb(slider, w_light_tile_slider_event_cb, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(slider, w_light_tile_slider_event_cb, LV_EVENT_RELEASED, ctx);

    light_apply_visual(card, false, 0, "OFF");
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
    light_apply_visual(instance->obj, is_on, brightness, is_on ? "ON" : "OFF");
}

void w_light_tile_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    light_apply_visual(instance->obj, false, 0, "unavailable");
}
