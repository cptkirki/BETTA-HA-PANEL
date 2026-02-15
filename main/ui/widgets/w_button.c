#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    lv_obj_t *card;
    lv_obj_t *title_label;
    lv_obj_t *state_label;
    lv_obj_t *action_switch;
    bool suppress_event;
    bool is_on;
    bool unavailable;
} w_button_ctx_t;

static const uint32_t W_BUTTON_SWITCH_TRACK_OFF_HEX = 0x3A3E43;
static const uint32_t W_BUTTON_SWITCH_TRACK_ON_HEX = APP_UI_COLOR_NAV_TAB_ACTIVE;
static const uint32_t W_BUTTON_SWITCH_KNOB_HEX = 0xEAF2FA;
static const lv_coord_t W_BUTTON_SWITCH_HEIGHT_PX = 40;

static bool state_is_on(const char *state)
{
    if (state == NULL) {
        return false;
    }
    return (strcmp(state, "on") == 0) || (strcmp(state, "open") == 0) || (strcmp(state, "playing") == 0) ||
           (strcmp(state, "home") == 0);
}

static void button_set_switch_checked(w_button_ctx_t *ctx, bool checked)
{
    if (ctx == NULL || ctx->action_switch == NULL) {
        return;
    }

    bool current = lv_obj_has_state(ctx->action_switch, LV_STATE_CHECKED);
    if (current == checked) {
        return;
    }

    ctx->suppress_event = true;
    if (checked) {
        lv_obj_add_state(ctx->action_switch, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ctx->action_switch, LV_STATE_CHECKED);
    }
    ctx->suppress_event = false;
}

static void button_layout_switch(lv_obj_t *card, w_button_ctx_t *ctx)
{
    if (card == NULL || ctx == NULL || ctx->title_label == NULL || ctx->state_label == NULL || ctx->action_switch == NULL) {
        return;
    }

    lv_obj_update_layout(card);

    const lv_coord_t side_inset =
#if APP_UI_TILE_LAYOUT_TUNED
        2;
#else
        0;
#endif
    const lv_coord_t top_gap =
#if APP_UI_TILE_LAYOUT_TUNED
        10;
#else
        8;
#endif
    const lv_coord_t bottom_gap =
#if APP_UI_TILE_LAYOUT_TUNED
        12;
#else
        10;
#endif
    const lv_coord_t min_height = 34;

    lv_coord_t content_w =
        lv_obj_get_width(card) - lv_obj_get_style_pad_left(card, LV_PART_MAIN) - lv_obj_get_style_pad_right(card, LV_PART_MAIN);
    lv_coord_t content_h =
        lv_obj_get_height(card) - lv_obj_get_style_pad_top(card, LV_PART_MAIN) - lv_obj_get_style_pad_bottom(card, LV_PART_MAIN);
    if (content_w < 24) {
        content_w = 24;
    }
    if (content_h < 48) {
        content_h = 48;
    }

    lv_coord_t top = lv_obj_get_y(ctx->state_label) + lv_obj_get_height(ctx->state_label) + top_gap;
    lv_coord_t bottom = lv_obj_get_y(ctx->title_label) - bottom_gap;
    if (top < 0) {
        top = 0;
    }
    if (bottom > content_h) {
        bottom = content_h;
    }
    if (bottom < (top + min_height)) {
        bottom = top + min_height;
        if (bottom > content_h) {
            bottom = content_h;
            top = bottom - min_height;
            if (top < 0) {
                top = 0;
            }
        }
    }

    lv_coord_t area_h = bottom - top;
    if (area_h < 20) {
        area_h = 20;
    }

    lv_coord_t max_w = content_w - (2 * side_inset);
    if (max_w < 20) {
        max_w = content_w;
    }
    if (max_w < 20) {
        max_w = 20;
    }

    lv_coord_t switch_h = (area_h < W_BUTTON_SWITCH_HEIGHT_PX) ? area_h : W_BUTTON_SWITCH_HEIGHT_PX;
    if (switch_h < 20) {
        switch_h = 20;
    }
    lv_coord_t switch_w = (switch_h * 23) / 10;
    if (switch_w > max_w) {
        switch_w = max_w;
        switch_h = (switch_w * 10) / 23;
        if (switch_h < 20) {
            switch_h = 20;
        }
        if (switch_h > area_h) {
            switch_h = area_h;
        }
    }

    lv_coord_t x = (content_w - switch_w) / 2;
    lv_coord_t y = top + (area_h - switch_h) / 2;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }

    lv_obj_set_pos(ctx->action_switch, x, y);
    lv_obj_set_size(ctx->action_switch, switch_w, switch_h);
}

static void button_apply_visual(lv_obj_t *card, w_button_ctx_t *ctx, bool is_on, bool unavailable, const char *status_text)
{
    if (card == NULL || ctx == NULL || ctx->title_label == NULL || ctx->state_label == NULL || ctx->action_switch == NULL) {
        return;
    }

    ctx->is_on = is_on;
    ctx->unavailable = unavailable;

    const lv_color_t card_bg =
        lv_color_hex((is_on && !unavailable) ? APP_UI_COLOR_CARD_BG_ON : APP_UI_COLOR_CARD_BG_OFF);
    const lv_color_t state_color = unavailable
                                       ? lv_color_hex(APP_UI_COLOR_TEXT_MUTED)
                                       : (is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF));
    const lv_color_t track_off =
        unavailable ? lv_color_hex(APP_UI_COLOR_CARD_BORDER) : lv_color_hex(W_BUTTON_SWITCH_TRACK_OFF_HEX);
    const lv_color_t track_on =
        unavailable ? lv_color_hex(APP_UI_COLOR_CARD_BORDER) : lv_color_hex(W_BUTTON_SWITCH_TRACK_ON_HEX);
    const lv_color_t knob_color =
        unavailable ? lv_color_hex(APP_UI_COLOR_TEXT_MUTED) : lv_color_hex(W_BUTTON_SWITCH_KNOB_HEX);

    lv_obj_set_style_bg_color(card, card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->title_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->state_label, state_color, LV_PART_MAIN);

    lv_obj_set_style_radius(ctx->action_switch, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ctx->action_switch, LV_RADIUS_CIRCLE, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ctx->action_switch, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ctx->action_switch, track_off, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ctx->action_switch, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ctx->action_switch, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ctx->action_switch, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ctx->action_switch, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_anim_duration(ctx->action_switch, 140, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ctx->action_switch, track_off, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ctx->action_switch, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ctx->action_switch, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ctx->action_switch, track_on, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ctx->action_switch, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_set_style_bg_color(ctx->action_switch, knob_color, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ctx->action_switch, LV_OPA_COVER, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ctx->action_switch, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ctx->action_switch, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    if (unavailable) {
        lv_obj_add_state(ctx->action_switch, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(ctx->action_switch, LV_STATE_DISABLED);
    }
    button_set_switch_checked(ctx, is_on && !unavailable);

    lv_label_set_text(ctx->state_label, status_text != NULL ? status_text : (is_on ? "ON" : "OFF"));
    button_layout_switch(card, ctx);
}

static void w_button_card_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    w_button_ctx_t *ctx = (w_button_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        if (ctx->unavailable) {
            return;
        }
        bool next = !ctx->is_on;
        button_apply_visual(ctx->card, ctx, next, false, next ? "ON" : "OFF");
        ui_bindings_toggle_entity(ctx->entity_id);
    } else if (code == LV_EVENT_DELETE) {
        free(ctx);
    }
}

static void w_button_switch_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    w_button_ctx_t *ctx = (w_button_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL || ctx->suppress_event || ctx->unavailable) {
        return;
    }

    lv_obj_t *sw = lv_event_get_target(event);
    if (sw == NULL) {
        return;
    }

    bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);
    button_apply_visual(ctx->card, ctx, checked, false, checked ? "ON" : "OFF");
    ui_bindings_toggle_entity(ctx->entity_id);
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
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -12);
#else
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -10);
#endif

    lv_obj_t *state_label = lv_label_create(card);
    lv_label_set_text(state_label, "OFF");
    lv_obj_set_style_text_font(state_label, LV_FONT_DEFAULT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(state_label, LV_ALIGN_TOP_LEFT, 0, 2);
#else
    lv_obj_align(state_label, LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    lv_obj_t *action_switch = lv_switch_create(card);
    lv_obj_clear_flag(action_switch, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(action_switch, 88, 40);
    lv_switch_set_orientation(action_switch, LV_SWITCH_ORIENTATION_HORIZONTAL);

    w_button_ctx_t *ctx = calloc(1, sizeof(w_button_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }
    snprintf(ctx->entity_id, sizeof(ctx->entity_id), "%s", def->entity_id);
    ctx->card = card;
    ctx->title_label = title;
    ctx->state_label = state_label;
    ctx->action_switch = action_switch;
    ctx->suppress_event = false;
    ctx->is_on = false;
    ctx->unavailable = false;

    lv_obj_add_event_cb(card, w_button_card_event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(card, w_button_card_event_cb, LV_EVENT_DELETE, ctx);
    lv_obj_add_event_cb(action_switch, w_button_switch_event_cb, LV_EVENT_VALUE_CHANGED, ctx);

    button_apply_visual(card, ctx, false, false, "OFF");
    out_instance->obj = card;
    out_instance->ctx = ctx;
    return ESP_OK;
}

void w_button_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }

    w_button_ctx_t *ctx = (w_button_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }

    const bool is_on = state_is_on(state->state);
    button_apply_visual(instance->obj, ctx, is_on, false, is_on ? "ON" : "OFF");
}

void w_button_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }

    w_button_ctx_t *ctx = (w_button_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }

    button_apply_visual(instance->obj, ctx, false, true, "unavailable");
}
