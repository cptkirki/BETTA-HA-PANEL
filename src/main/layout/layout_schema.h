#pragma once

#include <stdint.h>

#include "app_config.h"

typedef enum {
    LAYOUT_WIDGET_SENSOR = 0,
    LAYOUT_WIDGET_BUTTON,
    LAYOUT_WIDGET_SLIDER,
    LAYOUT_WIDGET_GRAPH,
    LAYOUT_WIDGET_LIGHT_TILE,
    LAYOUT_WIDGET_HEATING_TILE,
    LAYOUT_WIDGET_WEATHER_TILE,
    LAYOUT_WIDGET_WEATHER_3DAY,
} layout_widget_type_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} layout_rect_t;

typedef struct {
    char id[APP_MAX_WIDGET_ID_LEN];
    layout_widget_type_t type;
    layout_rect_t rect;
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    char title[APP_MAX_NAME_LEN];
} layout_widget_t;

typedef struct {
    char id[APP_MAX_PAGE_ID_LEN];
    char title[APP_MAX_NAME_LEN];
    uint16_t widget_count;
    layout_widget_t widgets[APP_MAX_WIDGETS_PER_PAGE];
} layout_page_t;

typedef struct {
    uint16_t version;
    uint16_t page_count;
    layout_page_t pages[APP_MAX_PAGES];
} layout_doc_t;
