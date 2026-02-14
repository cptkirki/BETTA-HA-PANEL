#include "ui/ui_widget_factory.h"

#include "ui/theme/theme_default.h"

esp_err_t w_graph_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, def->x, def->y);
    lv_obj_set_size(card, def->w, def->h);
    theme_default_style_card(card);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text_fmt(title, "%s", def->title[0] ? def->title : def->id);
    lv_obj_set_style_text_color(title, theme_default_color_text_muted(), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *hint = lv_label_create(card);
    lv_label_set_text(hint, "Graph: phase 2");
    lv_obj_set_style_text_color(hint, theme_default_color_text_primary(), LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);

    out_instance->obj = card;
    return ESP_OK;
}

void w_graph_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    (void)instance;
    (void)state;
}

void w_graph_mark_unavailable(ui_widget_instance_t *instance)
{
    (void)instance;
}
