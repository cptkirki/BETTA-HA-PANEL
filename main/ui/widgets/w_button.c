#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"

typedef enum {
    W_BUTTON_MODE_AUTO = 0,
    W_BUTTON_MODE_PLAY_PAUSE,
    W_BUTTON_MODE_STOP,
    W_BUTTON_MODE_NEXT,
    W_BUTTON_MODE_PREVIOUS,
} w_button_mode_t;

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    lv_obj_t *card;
    lv_obj_t *title_label;
    lv_obj_t *state_label;
    lv_obj_t *action_switch;
    lv_obj_t *action_icon;
    lv_color_t accent_color;
    w_button_mode_t mode;
    bool show_title;
    bool show_status;
    bool suppress_event;
    bool is_on;
    bool unavailable;
} w_button_ctx_t;

static const uint32_t W_BUTTON_SWITCH_TRACK_OFF_HEX = 0x3A3E43;
static const uint32_t W_BUTTON_SWITCH_ACCENT_DEFAULT_HEX = APP_UI_COLOR_NAV_TAB_ACTIVE;
static const uint32_t W_BUTTON_SWITCH_KNOB_HEX = 0xEAF2FA;
static const lv_coord_t W_BUTTON_SWITCH_HEIGHT_PX = 40;

static bool button_is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int button_hex_nibble(char c)
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

static bool button_parse_hex_color(const char *text, lv_color_t *out)
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
        if (!button_is_hex_digit(p[i])) {
            return false;
        }
    }

    int r_hi = button_hex_nibble(p[0]);
    int r_lo = button_hex_nibble(p[1]);
    int g_hi = button_hex_nibble(p[2]);
    int g_lo = button_hex_nibble(p[3]);
    int b_hi = button_hex_nibble(p[4]);
    int b_lo = button_hex_nibble(p[5]);
    if (r_hi < 0 || r_lo < 0 || g_hi < 0 || g_lo < 0 || b_hi < 0 || b_lo < 0) {
        return false;
    }

    uint32_t rgb = (uint32_t)(((r_hi << 4) | r_lo) << 16) | (uint32_t)(((g_hi << 4) | g_lo) << 8) |
                   (uint32_t)((b_hi << 4) | b_lo);
    *out = lv_color_hex(rgb);
    return true;
}

static bool state_is_unavailable(const char *state)
{
    if (state == NULL) {
        return false;
    }
    return strcmp(state, "unavailable") == 0 || strcmp(state, "unknown") == 0;
}

static bool state_is_on(const char *state)
{
    if (state == NULL) {
        return false;
    }
    return (strcmp(state, "on") == 0) || (strcmp(state, "open") == 0) || (strcmp(state, "playing") == 0) ||
           (strcmp(state, "home") == 0);
}

static bool button_entity_is_media_player(const char *entity_id)
{
    if (entity_id == NULL) {
        return false;
    }
    return strncmp(entity_id, "media_player.", strlen("media_player.")) == 0;
}

static w_button_mode_t button_mode_from_text(const char *mode_text)
{
    if (mode_text == NULL || mode_text[0] == '\0' || strcmp(mode_text, "auto") == 0) {
        return W_BUTTON_MODE_AUTO;
    }
    if (strcmp(mode_text, "play_pause") == 0) {
        return W_BUTTON_MODE_PLAY_PAUSE;
    }
    if (strcmp(mode_text, "stop") == 0) {
        return W_BUTTON_MODE_STOP;
    }
    if (strcmp(mode_text, "next") == 0) {
        return W_BUTTON_MODE_NEXT;
    }
    if (strcmp(mode_text, "previous") == 0) {
        return W_BUTTON_MODE_PREVIOUS;
    }
    return W_BUTTON_MODE_AUTO;
}

static bool button_mode_uses_switch(w_button_mode_t mode)
{
    return mode == W_BUTTON_MODE_AUTO;
}

static const char *button_icon_symbol(w_button_mode_t mode, bool is_on)
{
    switch (mode) {
    case W_BUTTON_MODE_PLAY_PAUSE:
        return is_on ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY;
    case W_BUTTON_MODE_STOP:
        return LV_SYMBOL_STOP;
    case W_BUTTON_MODE_NEXT:
        return LV_SYMBOL_NEXT;
    case W_BUTTON_MODE_PREVIOUS:
        return LV_SYMBOL_PREV;
    case W_BUTTON_MODE_AUTO:
    default:
        return "";
    }
}

static bool button_label_visible(lv_obj_t *obj)
{
    return obj != NULL && !lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static const lv_font_t *button_pick_icon_font(lv_coord_t available_h)
{
#if LV_FONT_MONTSERRAT_72
    if (available_h >= 92) {
        return &lv_font_montserrat_72;
    }
#endif
#if LV_FONT_MONTSERRAT_56
    if (available_h >= 74) {
        return &lv_font_montserrat_56;
    }
#endif
#if LV_FONT_MONTSERRAT_48
    if (available_h >= 64) {
        return &lv_font_montserrat_48;
    }
#endif
#if LV_FONT_MONTSERRAT_44
    if (available_h >= 58) {
        return &lv_font_montserrat_44;
    }
#endif
#if LV_FONT_MONTSERRAT_40
    if (available_h >= 52) {
        return &lv_font_montserrat_40;
    }
#endif
#if LV_FONT_MONTSERRAT_36
    if (available_h >= 46) {
        return &lv_font_montserrat_36;
    }
#endif
#if LV_FONT_MONTSERRAT_32
    if (available_h >= 40) {
        return &lv_font_montserrat_32;
    }
#endif
#if LV_FONT_MONTSERRAT_28
    if (available_h >= 34) {
        return &lv_font_montserrat_28;
    }
#endif
    return LV_FONT_DEFAULT;
}

static void button_calc_action_area(lv_obj_t *card, w_button_ctx_t *ctx, lv_coord_t top_gap, lv_coord_t bottom_gap,
    lv_coord_t min_height, lv_coord_t *out_content_w, lv_coord_t *out_top, lv_coord_t *out_area_h)
{
    if (card == NULL || ctx == NULL || out_content_w == NULL || out_top == NULL || out_area_h == NULL) {
        return;
    }

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

    lv_coord_t top = top_gap / 2;
    if (button_label_visible(ctx->state_label)) {
        top = lv_obj_get_y(ctx->state_label) + lv_obj_get_height(ctx->state_label) + top_gap;
    }

    lv_coord_t bottom = content_h - (bottom_gap / 2);
    if (button_label_visible(ctx->title_label)) {
        bottom = lv_obj_get_y(ctx->title_label) - bottom_gap;
    }

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

    *out_content_w = content_w;
    *out_top = top;
    *out_area_h = area_h;
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
    if (card == NULL || ctx == NULL || ctx->action_switch == NULL) {
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

    lv_coord_t content_w = 24;
    lv_coord_t top = 0;
    lv_coord_t area_h = 20;
    button_calc_action_area(card, ctx, top_gap, bottom_gap, min_height, &content_w, &top, &area_h);

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

static void button_layout_icon(lv_obj_t *card, w_button_ctx_t *ctx)
{
    if (card == NULL || ctx == NULL || ctx->action_icon == NULL) {
        return;
    }

    lv_obj_update_layout(card);

    const lv_coord_t top_gap_base =
#if APP_UI_TILE_LAYOUT_TUNED
        14;
#else
        12;
#endif
    const lv_coord_t bottom_gap_base =
#if APP_UI_TILE_LAYOUT_TUNED
        16;
#else
        14;
#endif
    const lv_coord_t min_height = 30;

    lv_coord_t top_gap = button_label_visible(ctx->state_label) ? top_gap_base : 4;
    lv_coord_t bottom_gap = button_label_visible(ctx->title_label) ? bottom_gap_base : 4;
    lv_coord_t content_w = 24;
    lv_coord_t top = 0;
    lv_coord_t area_h = 20;
    button_calc_action_area(card, ctx, top_gap, bottom_gap, min_height, &content_w, &top, &area_h);

    lv_coord_t target_icon_h = (area_h * 9) / 10;
    if (target_icon_h < 20) {
        target_icon_h = 20;
    }
    lv_obj_set_style_text_font(ctx->action_icon, button_pick_icon_font(target_icon_h), LV_PART_MAIN);
    lv_obj_set_width(ctx->action_icon, content_w);
    lv_obj_update_layout(ctx->action_icon);

    lv_coord_t icon_h = lv_obj_get_height(ctx->action_icon);
    if (icon_h < 20) {
        icon_h = 20;
    }
    lv_coord_t y = top + (area_h - icon_h) / 2;
    if (y < 0) {
        y = 0;
    }

    lv_obj_set_pos(ctx->action_icon, 0, y);
}

static void button_apply_visual(lv_obj_t *card, w_button_ctx_t *ctx, bool is_on, bool unavailable, const char *status_text)
{
    if (card == NULL || ctx == NULL || ctx->title_label == NULL || ctx->state_label == NULL) {
        return;
    }

    ctx->is_on = is_on;
    ctx->unavailable = unavailable;

    const lv_color_t card_bg =
        lv_color_hex((is_on && !unavailable) ? APP_UI_COLOR_CARD_BG_ON : APP_UI_COLOR_CARD_BG_OFF);
    const lv_color_t state_color = unavailable
                                       ? lv_color_hex(APP_UI_COLOR_TEXT_MUTED)
                                       : (is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF));

    lv_obj_set_style_bg_color(card, card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    if (ctx->show_title) {
        lv_obj_clear_flag(ctx->title_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(ctx->title_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    } else {
        lv_obj_add_flag(ctx->title_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (ctx->show_status) {
        lv_obj_clear_flag(ctx->state_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(ctx->state_label, state_color, LV_PART_MAIN);
    } else {
        lv_obj_add_flag(ctx->state_label, LV_OBJ_FLAG_HIDDEN);
    }

    if (button_mode_uses_switch(ctx->mode)) {
        if (ctx->action_switch != NULL) {
            const lv_color_t track_off =
                unavailable ? lv_color_hex(APP_UI_COLOR_CARD_BORDER) : lv_color_hex(W_BUTTON_SWITCH_TRACK_OFF_HEX);
            const lv_color_t track_on =
                unavailable ? lv_color_hex(APP_UI_COLOR_CARD_BORDER) : ctx->accent_color;
            const lv_color_t knob_color =
                unavailable ? lv_color_hex(APP_UI_COLOR_TEXT_MUTED) : lv_color_hex(W_BUTTON_SWITCH_KNOB_HEX);

            lv_obj_clear_flag(ctx->action_switch, LV_OBJ_FLAG_HIDDEN);

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
            button_layout_switch(card, ctx);
        }

        if (ctx->action_icon != NULL) {
            lv_obj_add_flag(ctx->action_icon, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (ctx->action_switch != NULL) {
            lv_obj_add_flag(ctx->action_switch, LV_OBJ_FLAG_HIDDEN);
        }

        if (ctx->action_icon != NULL) {
            lv_obj_clear_flag(ctx->action_icon, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(ctx->action_icon, button_icon_symbol(ctx->mode, is_on && !unavailable));
            lv_obj_set_style_text_color(ctx->action_icon, unavailable ? lv_color_hex(APP_UI_COLOR_TEXT_MUTED) : ctx->accent_color,
                LV_PART_MAIN);
            button_layout_icon(card, ctx);
        }
    }

    if (ctx->show_status) {
        lv_label_set_text(ctx->state_label, status_text != NULL ? status_text : (is_on ? "ON" : "OFF"));
    }
}

static const char *button_status_text_for_state(const w_button_ctx_t *ctx, const ha_state_t *state, bool is_on, bool unavailable)
{
    if (ctx == NULL) {
        return unavailable ? "unavailable" : (is_on ? "ON" : "OFF");
    }
    if (unavailable) {
        return "unavailable";
    }
    if (ctx->mode == W_BUTTON_MODE_AUTO) {
        return is_on ? "ON" : "OFF";
    }
    if (state != NULL && state->state[0] != '\0') {
        return state->state;
    }
    return is_on ? "playing" : "paused";
}

static void button_run_primary_action(w_button_ctx_t *ctx)
{
    if (ctx == NULL || ctx->unavailable) {
        return;
    }

    if (button_mode_uses_switch(ctx->mode)) {
        bool next = !ctx->is_on;
        button_apply_visual(ctx->card, ctx, next, false, next ? "ON" : "OFF");
        ui_bindings_toggle_entity(ctx->entity_id);
        return;
    }

    if (ctx->mode == W_BUTTON_MODE_PLAY_PAUSE) {
        bool next = !ctx->is_on;
        button_apply_visual(ctx->card, ctx, next, false, next ? "playing" : "paused");
        ui_bindings_media_player_action(ctx->entity_id, UI_BINDINGS_MEDIA_ACTION_PLAY_PAUSE);
    } else if (ctx->mode == W_BUTTON_MODE_STOP) {
        ui_bindings_media_player_action(ctx->entity_id, UI_BINDINGS_MEDIA_ACTION_STOP);
    } else if (ctx->mode == W_BUTTON_MODE_NEXT) {
        ui_bindings_media_player_action(ctx->entity_id, UI_BINDINGS_MEDIA_ACTION_NEXT);
    } else if (ctx->mode == W_BUTTON_MODE_PREVIOUS) {
        ui_bindings_media_player_action(ctx->entity_id, UI_BINDINGS_MEDIA_ACTION_PREVIOUS);
    }
}

static void w_button_card_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    w_button_ctx_t *ctx = (w_button_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        button_run_primary_action(ctx);
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
    if (ctx == NULL || ctx->suppress_event || ctx->unavailable || !button_mode_uses_switch(ctx->mode)) {
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

    const bool is_media_player = button_entity_is_media_player(def->entity_id);
    const char *title_text = def->title;
    if (!is_media_player && (title_text == NULL || title_text[0] == '\0')) {
        title_text = def->id;
    }
    if (title_text == NULL) {
        title_text = "";
    }

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, title_text);
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

    lv_obj_t *action_icon = lv_label_create(card);
    lv_obj_set_style_text_font(action_icon, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_align(action_icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(action_icon, "");
    lv_obj_add_flag(action_icon, LV_OBJ_FLAG_HIDDEN);

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
    ctx->action_icon = action_icon;
    ctx->accent_color = lv_color_hex(W_BUTTON_SWITCH_ACCENT_DEFAULT_HEX);
    ctx->mode = button_mode_from_text(def->button_mode);
    ctx->show_title = title_text[0] != '\0';
    ctx->show_status = !is_media_player;
    if (ctx->mode != W_BUTTON_MODE_AUTO && !is_media_player) {
        ctx->mode = W_BUTTON_MODE_AUTO;
    }
    ctx->suppress_event = false;
    ctx->is_on = false;
    ctx->unavailable = false;

    lv_color_t parsed_color = lv_color_hex(0);
    if (button_parse_hex_color(def->button_accent_color, &parsed_color)) {
        ctx->accent_color = parsed_color;
    }

    if (!ctx->show_title) {
        lv_obj_add_flag(title, LV_OBJ_FLAG_HIDDEN);
    }
    if (!ctx->show_status) {
        lv_obj_add_flag(state_label, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_add_event_cb(card, w_button_card_event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(card, w_button_card_event_cb, LV_EVENT_DELETE, ctx);
    lv_obj_add_event_cb(action_switch, w_button_switch_event_cb, LV_EVENT_VALUE_CHANGED, ctx);

    button_apply_visual(card, ctx, false, false, (ctx->mode == W_BUTTON_MODE_AUTO) ? "OFF" : "paused");
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

    const bool unavailable = state_is_unavailable(state->state);
    const bool is_on = state_is_on(state->state);
    const char *status_text = button_status_text_for_state(ctx, state, is_on, unavailable);
    button_apply_visual(instance->obj, ctx, is_on, unavailable, status_text);
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
