#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"

#if LV_FONT_MONTSERRAT_34
#define BUTTON_ACTION_ICON_FONT (&lv_font_montserrat_34)
#elif LV_FONT_MONTSERRAT_28
#define BUTTON_ACTION_ICON_FONT (&lv_font_montserrat_28)
#elif LV_FONT_MONTSERRAT_24
#define BUTTON_ACTION_ICON_FONT (&lv_font_montserrat_24)
#else
#define BUTTON_ACTION_ICON_FONT LV_FONT_DEFAULT
#endif

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    lv_obj_t *title_label;
    lv_obj_t *state_label;
    lv_obj_t *action_surface;
    lv_obj_t *action_icon;
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

static void button_layout_action_surface(lv_obj_t *card, w_button_ctx_t *ctx)
{
    if (card == NULL || ctx == NULL || ctx->title_label == NULL || ctx->state_label == NULL || ctx->action_surface == NULL) {
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
    const lv_coord_t min_height = 44;

    lv_coord_t pad_left = lv_obj_get_style_pad_left(card, LV_PART_MAIN);
    lv_coord_t pad_right = lv_obj_get_style_pad_right(card, LV_PART_MAIN);
    lv_coord_t pad_top = lv_obj_get_style_pad_top(card, LV_PART_MAIN);
    lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(card, LV_PART_MAIN);

    lv_coord_t content_w = lv_obj_get_width(card) - pad_left - pad_right;
    lv_coord_t content_h = lv_obj_get_height(card) - pad_top - pad_bottom;
    if (content_w < 24) {
        content_w = 24;
    }
    if (content_h < 48) {
        content_h = 48;
    }

    lv_coord_t x = side_inset;
    lv_coord_t width = content_w - (2 * side_inset);
    if (width < 24) {
        width = 24;
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
        top = bottom - min_height;
        if (top < 0) {
            top = 0;
            bottom = min_height;
        }
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

    lv_obj_set_pos(ctx->action_surface, x, top);
    lv_obj_set_size(ctx->action_surface, width, bottom - top);
    if (ctx->action_icon != NULL) {
        lv_obj_center(ctx->action_icon);
    }
}

static void button_apply_visual(lv_obj_t *card, w_button_ctx_t *ctx, bool is_on, const char *status_text)
{
    if (card == NULL || ctx == NULL || ctx->title_label == NULL || ctx->state_label == NULL || ctx->action_surface == NULL ||
        ctx->action_icon == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(
        card, is_on ? lv_color_hex(APP_UI_COLOR_CARD_BG_ON) : lv_color_hex(APP_UI_COLOR_CARD_BG_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->title_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(
        ctx->state_label, is_on ? lv_color_hex(APP_UI_COLOR_STATE_ON) : lv_color_hex(APP_UI_COLOR_STATE_OFF), LV_PART_MAIN);

    if (is_on) {
        lv_obj_set_style_bg_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_NAV_BTN_BG_ACTIVE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_NAV_BTN_BG_IDLE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(ctx->action_surface, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ctx->action_surface, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ctx->action_surface, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ctx->action_surface, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_NAV_TAB_ACTIVE), LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_style_bg_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_NAV_BTN_BG_IDLE), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_CARD_BG_ON), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_dir(ctx->action_surface, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(ctx->action_surface, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(ctx->action_surface, 1, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(ctx->action_surface, LV_OPA_80, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_NAV_HOME_ACTIVE), LV_PART_MAIN | LV_STATE_PRESSED);
    } else {
        /* Keep OFF state flat: one solid fill color without depth gradient. */
        lv_obj_set_style_bg_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_CONTENT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_CONTENT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(ctx->action_surface, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ctx->action_surface, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ctx->action_surface, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ctx->action_surface, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_CARD_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_style_bg_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_CONTENT_BG), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_CONTENT_BG), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_dir(ctx->action_surface, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(ctx->action_surface, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(ctx->action_surface, 1, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_opa(ctx->action_surface, LV_OPA_80, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(
            ctx->action_surface, lv_color_hex(APP_UI_COLOR_CARD_BORDER), LV_PART_MAIN | LV_STATE_PRESSED);
    }

    lv_obj_set_style_radius(ctx->action_surface, APP_UI_CARD_RADIUS - 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ctx->action_surface, APP_UI_CARD_RADIUS - 6, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_pad_all(ctx->action_surface, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ctx->action_surface, 0, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_set_style_text_font(ctx->action_icon, BUTTON_ACTION_ICON_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(
        ctx->action_icon,
        is_on ? lv_color_hex(APP_UI_COLOR_NAV_TAB_ACTIVE) : lv_color_hex(APP_UI_COLOR_TEXT_SOFT),
        LV_PART_MAIN);
    lv_label_set_text(ctx->action_icon, LV_SYMBOL_POWER);

    lv_label_set_text(ctx->state_label, status_text != NULL ? status_text : (is_on ? "ON" : "OFF"));
    button_layout_action_surface(card, ctx);
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

    lv_obj_t *action_surface = lv_obj_create(card);
    lv_obj_remove_style_all(action_surface);
    lv_obj_set_size(action_surface, def->w - 36, def->h - 88);
    lv_obj_add_flag(action_surface, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(action_surface, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(action_surface, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *action_icon = lv_label_create(action_surface);
    lv_label_set_text(action_icon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_font(action_icon, BUTTON_ACTION_ICON_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_align(action_icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(action_icon);

    w_button_ctx_t *ctx = calloc(1, sizeof(w_button_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }
    snprintf(ctx->entity_id, sizeof(ctx->entity_id), "%s", def->entity_id);
    ctx->title_label = title;
    ctx->state_label = state_label;
    ctx->action_surface = action_surface;
    ctx->action_icon = action_icon;

    lv_obj_add_event_cb(card, w_button_event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(card, w_button_event_cb, LV_EVENT_DELETE, ctx);
    lv_obj_add_event_cb(action_surface, w_button_event_cb, LV_EVENT_CLICKED, ctx);

    button_apply_visual(card, ctx, false, "OFF");
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
    button_apply_visual(instance->obj, ctx, is_on, is_on ? "ON" : "OFF");
}

void w_button_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    w_button_ctx_t *ctx = (w_button_ctx_t *)instance->ctx;
    button_apply_visual(instance->obj, ctx, false, "unavailable");
}
