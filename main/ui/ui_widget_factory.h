#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "lvgl.h"

#include "app_config.h"
#include "ha/ha_model.h"

typedef struct {
    char id[APP_MAX_WIDGET_ID_LEN];
    char type[16];
    char title[APP_MAX_NAME_LEN];
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    char secondary_entity_id[APP_MAX_ENTITY_ID_LEN];
    char slider_direction[APP_MAX_UI_OPTION_LEN];
    char slider_accent_color[APP_MAX_COLOR_STR_LEN];
    int x;
    int y;
    int w;
    int h;
} ui_widget_def_t;

typedef struct {
    char id[APP_MAX_WIDGET_ID_LEN];
    char type[16];
    char title[APP_MAX_NAME_LEN];
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    char secondary_entity_id[APP_MAX_ENTITY_ID_LEN];
    char slider_direction[APP_MAX_UI_OPTION_LEN];
    char slider_accent_color[APP_MAX_COLOR_STR_LEN];
    void *ctx;
    lv_obj_t *obj;
} ui_widget_instance_t;

esp_err_t ui_widget_factory_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance);
void ui_widget_factory_apply_state(ui_widget_instance_t *instance, const ha_state_t *state);
void ui_widget_factory_mark_unavailable(ui_widget_instance_t *instance);
