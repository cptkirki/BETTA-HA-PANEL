#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    bool suppress_switch_event;
} w_button_ctx_t;

static bool state_is_on(const char *state)
{
    if (state == NULL) {
        return false;
    }
    return (strcmp(state, "on") == 0) || (strcmp(state, "open") == 0) || (strcmp(state, "playing") == 0) ||
           (strcmp(state, "home") == 0);
}

static void w_button_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    w_button_ctx_t *ctx = (w_button_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }
    if (code == LV_EVENT_CLICKED) {
        ui_bindings_toggle_entity(ctx->entity_id);
    } else if (code == LV_EVENT_DELETE) {
        free(ctx);
    }
}

static void w_button_switch_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    w_button_ctx_t *ctx = (w_button_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL || ctx->suppress_switch_event) {
        return;
    }
    ui_bindings_toggle_entity(ctx->entity_id);
}

static void button_position_switch_between_state_and_title(lv_obj_t *card)
{
    if (card == NULL) {
        return;
    }

    lv_obj_t *title = lv_obj_get_child(card, 0);
    lv_obj_t *state_label = lv_obj_get_child(card, 1);
    lv_obj_t *sw = lv_obj_get_child(card, 2);
    if (title == NULL || state_label == NULL || sw == NULL) {
        return;
    }

    lv_obj_update_layout(card);

    const lv_coord_t gap =
#if APP_UI_TILE_LAYOUT_TUNED
        12;
#else
        10;
#endif

    lv_coord_t top = lv_obj_get_y(state_label) + lv_obj_get_height(state_label) + gap;
    lv_coord_t bottom = lv_obj_get_y(title) - gap;
    lv_coord_t sw_h = lv_obj_get_height(sw);
    lv_coord_t y = top;
    lv_coord_t room = bottom - top;

    if (room >= sw_h) {
        y = top + (room - sw_h) / 2;
    }

    lv_obj_align(sw, LV_ALIGN_TOP_MID, 0, y);
}

static void button_apply_visual(lv_obj_t *card, bool is_on, const char *status_text, w_button_ctx_t *ctx)
{
    if (card == NULL) {
        return;
    }

    lv_obj_t *title = lv_obj_get_child(card, 0);
    lv_obj_t *state_label = lv_obj_get_child(card, 1);
    lv_obj_t *sw = lv_obj_get_child(card, 2);
    if (title == NULL || state_label == NULL || sw == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(
        card, is_on ? lv_color_hex(APP_UI_COLOR_CARD_BG_ON) : lv_color_hex(APP_UI_COLOR_CARD_BG_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(
        state_label, is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF), LV_PART_MAIN);

    lv_obj_set_style_bg_color(
        sw, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_TRACK_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_TRACK_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        sw, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_IND_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_IND_OFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(
        sw, is_on ? lv_color_hex(APP_UI_COLOR_LIGHT_KNOB_ON) : lv_color_hex(APP_UI_COLOR_LIGHT_KNOB_OFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_KNOB);

    if (ctx != NULL) {
        ctx->suppress_switch_event = true;
    }
    if (is_on) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(sw, LV_STATE_CHECKED);
    }
    if (ctx != NULL) {
        ctx->suppress_switch_event = false;
    }

    lv_label_set_text(state_label, status_text != NULL ? status_text : (is_on ? "ON" : "OFF"));
    button_position_switch_between_state_and_title(card);
}

esp_err_t w_button_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
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

    lv_obj_t *sw = lv_switch_create(card);
    lv_obj_set_size(sw, 72, 38);
    lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_align(sw, LV_ALIGN_CENTER, 0, 6);
    lv_obj_clear_flag(sw, LV_OBJ_FLAG_EVENT_BUBBLE);

    w_button_ctx_t *ctx = calloc(1, sizeof(w_button_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }
    snprintf(ctx->entity_id, sizeof(ctx->entity_id), "%s", def->entity_id);
    lv_obj_add_event_cb(card, w_button_event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(card, w_button_event_cb, LV_EVENT_DELETE, ctx);
    lv_obj_add_event_cb(sw, w_button_switch_event_cb, LV_EVENT_VALUE_CHANGED, ctx);

    button_apply_visual(card, false, "OFF", ctx);
    out_instance->obj = card;
    out_instance->ctx = ctx;
    return ESP_OK;
}

void w_button_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }

    const bool is_on = state_is_on(state->state);
    w_button_ctx_t *ctx = (w_button_ctx_t *)instance->ctx;
    button_apply_visual(instance->obj, is_on, is_on ? "ON" : "OFF", ctx);
}

void w_button_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    w_button_ctx_t *ctx = (w_button_ctx_t *)instance->ctx;
    button_apply_visual(instance->obj, false, "unavailable", ctx);
}
