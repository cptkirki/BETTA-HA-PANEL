/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "ui/ui_widget_factory.h"

#include <stdio.h>
#include <string.h>

esp_err_t w_sensor_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_sensor_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_sensor_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_button_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_button_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_button_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_slider_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_slider_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_slider_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_graph_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_graph_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_graph_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_empty_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_empty_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_empty_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_light_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_light_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_light_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_heating_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_heating_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_heating_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t w_weather_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void w_weather_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void w_weather_tile_mark_unavailable(ui_widget_instance_t *instance);

esp_err_t ui_widget_factory_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
{
    if (def == NULL || parent == NULL || out_instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out_instance, 0, sizeof(*out_instance));
    snprintf(out_instance->id, sizeof(out_instance->id), "%s", def->id);
    snprintf(out_instance->type, sizeof(out_instance->type), "%s", def->type);
    snprintf(out_instance->title, sizeof(out_instance->title), "%s", def->title);
    snprintf(out_instance->entity_id, sizeof(out_instance->entity_id), "%s", def->entity_id);
    snprintf(out_instance->secondary_entity_id, sizeof(out_instance->secondary_entity_id), "%s", def->secondary_entity_id);
    snprintf(out_instance->slider_direction, sizeof(out_instance->slider_direction), "%s", def->slider_direction);
    snprintf(out_instance->slider_accent_color, sizeof(out_instance->slider_accent_color), "%s", def->slider_accent_color);
    snprintf(out_instance->button_accent_color, sizeof(out_instance->button_accent_color), "%s", def->button_accent_color);
    snprintf(out_instance->button_mode, sizeof(out_instance->button_mode), "%s", def->button_mode);
    snprintf(out_instance->graph_line_color, sizeof(out_instance->graph_line_color), "%s", def->graph_line_color);
    out_instance->graph_point_count = def->graph_point_count;
    out_instance->graph_time_window_min = def->graph_time_window_min;
    out_instance->ctx = NULL;

    if (strcmp(def->type, "sensor") == 0) {
        return w_sensor_create(def, parent, out_instance);
    }
    if (strcmp(def->type, "button") == 0) {
        return w_button_create(def, parent, out_instance);
    }
    if (strcmp(def->type, "slider") == 0) {
        return w_slider_create(def, parent, out_instance);
    }
    if (strcmp(def->type, "graph") == 0) {
        return w_graph_create(def, parent, out_instance);
    }
    if (strcmp(def->type, "empty_tile") == 0) {
        return w_empty_tile_create(def, parent, out_instance);
    }
    if (strcmp(def->type, "light_tile") == 0) {
        return w_light_tile_create(def, parent, out_instance);
    }
    if (strcmp(def->type, "heating_tile") == 0) {
        return w_heating_tile_create(def, parent, out_instance);
    }
    if (strcmp(def->type, "weather_tile") == 0 || strcmp(def->type, "weather_3day") == 0) {
        return w_weather_tile_create(def, parent, out_instance);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

void ui_widget_factory_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }
    if (strcmp(instance->type, "sensor") == 0) {
        w_sensor_apply_state(instance, state);
    } else if (strcmp(instance->type, "button") == 0) {
        w_button_apply_state(instance, state);
    } else if (strcmp(instance->type, "slider") == 0) {
        w_slider_apply_state(instance, state);
    } else if (strcmp(instance->type, "graph") == 0) {
        w_graph_apply_state(instance, state);
    } else if (strcmp(instance->type, "empty_tile") == 0) {
        w_empty_tile_apply_state(instance, state);
    } else if (strcmp(instance->type, "light_tile") == 0) {
        w_light_tile_apply_state(instance, state);
    } else if (strcmp(instance->type, "heating_tile") == 0) {
        w_heating_tile_apply_state(instance, state);
    } else if (strcmp(instance->type, "weather_tile") == 0 || strcmp(instance->type, "weather_3day") == 0) {
        w_weather_tile_apply_state(instance, state);
    }
}

void ui_widget_factory_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    if (strcmp(instance->type, "sensor") == 0) {
        w_sensor_mark_unavailable(instance);
    } else if (strcmp(instance->type, "button") == 0) {
        w_button_mark_unavailable(instance);
    } else if (strcmp(instance->type, "slider") == 0) {
        w_slider_mark_unavailable(instance);
    } else if (strcmp(instance->type, "graph") == 0) {
        w_graph_mark_unavailable(instance);
    } else if (strcmp(instance->type, "empty_tile") == 0) {
        w_empty_tile_mark_unavailable(instance);
    } else if (strcmp(instance->type, "light_tile") == 0) {
        w_light_tile_mark_unavailable(instance);
    } else if (strcmp(instance->type, "heating_tile") == 0) {
        w_heating_tile_mark_unavailable(instance);
    } else if (strcmp(instance->type, "weather_tile") == 0 || strcmp(instance->type, "weather_3day") == 0) {
        w_weather_tile_mark_unavailable(instance);
    }
}
