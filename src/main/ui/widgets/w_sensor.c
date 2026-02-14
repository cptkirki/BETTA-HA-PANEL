#include "ui/ui_widget_factory.h"

#include <stdio.h>

#include "cJSON.h"

#include "ui/theme/theme_default.h"

esp_err_t w_sensor_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, def->x, def->y);
    lv_obj_set_size(card, def->w, def->h);
    theme_default_style_card(card);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->id);
    lv_obj_set_style_text_color(title, theme_default_color_text_muted(), LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_set_width(title, def->w - 32);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
#else
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    lv_obj_t *value = lv_label_create(card);
    lv_label_set_text(value, "--");
    lv_obj_set_style_text_color(value, theme_default_color_text_primary(), LV_PART_MAIN);
    lv_obj_set_style_text_font(value, LV_FONT_DEFAULT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_set_width(value, def->w - 32);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(value, LV_ALIGN_CENTER, 0, 18);
#else
    lv_obj_align(value, LV_ALIGN_BOTTOM_LEFT, 0, 0);
#endif

    out_instance->obj = card;
    return ESP_OK;
}

static void set_sensor_value_label(ui_widget_instance_t *instance, const char *text)
{
    lv_obj_t *value = lv_obj_get_child(instance->obj, 1);
    if (value != NULL) {
        lv_label_set_text(value, text);
    }
}

void w_sensor_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }

    char value_text[96] = {0};
    const char *unit = NULL;
    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *unit_item = cJSON_GetObjectItemCaseSensitive(attrs, "unit_of_measurement");
        if (cJSON_IsString(unit_item) && unit_item->valuestring != NULL) {
            unit = unit_item->valuestring;
        }
    }
    if (unit != NULL && unit[0] != '\0') {
        snprintf(value_text, sizeof(value_text), "%s %s", state->state, unit);
    } else {
        snprintf(value_text, sizeof(value_text), "%s", state->state);
    }
    if (attrs != NULL) {
        cJSON_Delete(attrs);
    }

    set_sensor_value_label(instance, value_text);
}

void w_sensor_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    set_sensor_value_label(instance, "unavailable");
}
