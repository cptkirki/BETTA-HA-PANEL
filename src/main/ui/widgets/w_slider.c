#include "ui/ui_widget_factory.h"

#include <stdio.h>
#include <stdlib.h>

#include "ui/theme/theme_default.h"
#include "ui/ui_bindings.h"

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    lv_obj_t *value_label;
} w_slider_ctx_t;

static void slider_set_value_label(lv_obj_t *label, int value)
{
    if (label == NULL) {
        return;
    }
    char text[24] = {0};
    snprintf(text, sizeof(text), "%d %%", value);
    lv_label_set_text(label, text);
}

static void w_slider_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    w_slider_ctx_t *ctx = (w_slider_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *slider = lv_event_get_target(event);
        int value = lv_slider_get_value(slider);
        slider_set_value_label(ctx->value_label, value);
    } else if (code == LV_EVENT_RELEASED) {
        lv_obj_t *slider = lv_event_get_target(event);
        int value = lv_slider_get_value(slider);
        ui_bindings_set_slider_value(ctx->entity_id, value);
    } else if (code == LV_EVENT_DELETE) {
        free(ctx);
    }
}

esp_err_t w_slider_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, def->x, def->y);
    lv_obj_set_size(card, def->w, def->h);
    theme_default_style_card(card);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->id);
    lv_obj_set_style_text_color(title, theme_default_color_text_muted(), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *slider = lv_slider_create(card);
    lv_obj_set_width(slider, def->w - 30);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 6);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);

    lv_obj_t *value = lv_label_create(card);
    slider_set_value_label(value, 0);
    lv_obj_set_style_text_color(value, theme_default_color_text_primary(), LV_PART_MAIN);
    lv_obj_align(value, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    w_slider_ctx_t *ctx = calloc(1, sizeof(w_slider_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }
    snprintf(ctx->entity_id, sizeof(ctx->entity_id), "%s", def->entity_id);
    ctx->value_label = value;

    lv_obj_add_event_cb(slider, w_slider_event_cb, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(slider, w_slider_event_cb, LV_EVENT_RELEASED, ctx);
    lv_obj_add_event_cb(slider, w_slider_event_cb, LV_EVENT_DELETE, ctx);

    out_instance->obj = card;
    return ESP_OK;
}

void w_slider_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }
    lv_obj_t *slider = lv_obj_get_child(instance->obj, 1);
    lv_obj_t *value = lv_obj_get_child(instance->obj, 2);
    if (slider == NULL || value == NULL) {
        return;
    }

    int value_int = atoi(state->state);
    if (value_int < 0) {
        value_int = 0;
    } else if (value_int > 100) {
        value_int = 100;
    }
    lv_slider_set_value(slider, value_int, LV_ANIM_OFF);
    slider_set_value_label(value, value_int);
}

void w_slider_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    lv_obj_t *value = lv_obj_get_child(instance->obj, 2);
    if (value != NULL) {
        lv_label_set_text(value, "unavailable");
    }
}
