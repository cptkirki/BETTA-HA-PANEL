/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "ui/ui_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "app_events.h"
#include "drivers/display_init.h"
#include "ha/ha_client.h"
#include "ha/ha_model.h"
#include "layout/layout_store.h"
#include "net/wifi_mgr.h"
#include "ui/fonts/mdi_font_registry.h"
#include "ui/ui_pages.h"
#include "ui/ui_widget_factory.h"
#include "ui/theme/theme_default.h"
#include "util/log_tags.h"

#define UI_MODEL_RECONCILE_INTERVAL_MS 1000

static ui_widget_instance_t s_widgets[APP_MAX_WIDGETS_TOTAL];
static size_t s_widget_count = 0;
static TaskHandle_t s_ui_task = NULL;
static bool s_initialized = false;
static int64_t s_last_topbar_refresh_ms = 0;
static int64_t s_last_model_reconcile_ms = 0;
static uint32_t s_last_model_revision = 0;
static bool s_model_reconcile_pending = false;
static bool s_pending_state_reconcile = false;
static bool s_pending_topbar_refresh = false;
static uint32_t s_deferred_event_count = 0;
static int64_t s_deferred_event_log_ms = 0;
typedef struct {
    bool valid;
    int minute;
    int hour;
    int day;
    int month;
    int year;
    bool wifi_connected;
    bool wifi_setup_ap_active;
    bool ha_connected;
    bool ha_initial_sync_done;
} ui_topbar_cache_t;
static ui_topbar_cache_t s_topbar_cache = {0};
#if APP_UI_TEST_WEATHER_ICON_OVERLAY
static lv_obj_t *s_weather_icon_overlay = NULL;
#endif

typedef struct {
    int min_w;
    int min_h;
    int max_w;
    int max_h;
} ui_widget_size_limits_t;

static ui_widget_size_limits_t ui_runtime_widget_size_limits(const char *type)
{
    ui_widget_size_limits_t limits = {
        .min_w = 60,
        .min_h = 60,
        .max_w = APP_CONTENT_BOX_WIDTH,
        .max_h = APP_CONTENT_BOX_HEIGHT,
    };

    if (type == NULL) {
        return limits;
    }

    if (strcmp(type, "sensor") == 0) {
        limits.min_w = 120;
        limits.min_h = 80;
    } else if (strcmp(type, "button") == 0) {
        limits.min_w = 100;
        limits.min_h = 100;
        limits.max_w = 480;
        limits.max_h = 320;
    } else if (strcmp(type, "slider") == 0) {
        limits.min_w = 100;
        limits.min_h = 100;
    } else if (strcmp(type, "graph") == 0) {
        limits.min_w = 220;
        limits.min_h = 140;
    } else if (strcmp(type, "empty_tile") == 0) {
        limits.min_w = 120;
        limits.min_h = 80;
    } else if (strcmp(type, "light_tile") == 0) {
        limits.min_w = 180;
        limits.min_h = 180;
        limits.max_w = 480;
        limits.max_h = 480;
    } else if (strcmp(type, "heating_tile") == 0) {
        limits.min_w = 220;
        limits.min_h = 200;
        limits.max_w = 480;
        limits.max_h = 480;
    } else if (strcmp(type, "weather_tile") == 0) {
        limits.min_w = 220;
        limits.min_h = 200;
        limits.max_w = 480;
        limits.max_h = 480;
    } else if (strcmp(type, "weather_3day") == 0) {
        limits.min_w = 260;
        limits.min_h = 220;
        limits.max_w = 640;
        limits.max_h = 420;
    }

    if (limits.max_w > APP_CONTENT_BOX_WIDTH) {
        limits.max_w = APP_CONTENT_BOX_WIDTH;
    }
    if (limits.max_h > APP_CONTENT_BOX_HEIGHT) {
        limits.max_h = APP_CONTENT_BOX_HEIGHT;
    }
    return limits;
}

static void ui_runtime_clamp_widget_rect(ui_widget_def_t *def)
{
    if (def == NULL) {
        return;
    }

    ui_widget_size_limits_t limits = ui_runtime_widget_size_limits(def->type);

    if (def->w < limits.min_w) {
        def->w = limits.min_w;
    }
    if (def->h < limits.min_h) {
        def->h = limits.min_h;
    }
    if (def->w > limits.max_w) {
        def->w = limits.max_w;
    }
    if (def->h > limits.max_h) {
        def->h = limits.max_h;
    }

    if (def->x < 0) {
        def->x = 0;
    }
    if (def->y < 0) {
        def->y = 0;
    }

    if (def->x + def->w > APP_CONTENT_BOX_WIDTH) {
        def->x = APP_CONTENT_BOX_WIDTH - def->w;
    }
    if (def->y + def->h > APP_CONTENT_BOX_HEIGHT) {
        def->y = APP_CONTENT_BOX_HEIGHT - def->h;
    }

    if (def->x < 0) {
        def->x = 0;
    }
    if (def->y < 0) {
        def->y = 0;
    }
}

static void ui_runtime_refresh_topbar(void)
{
    time_t now = time(NULL);
    struct tm info = {0};
    localtime_r(&now, &info);

    bool wifi_connected = wifi_mgr_is_connected();
    bool wifi_setup_ap_active = wifi_mgr_is_setup_ap_active();
    bool ha_connected = ha_client_is_connected();
    bool ha_initial_sync_done = ha_client_is_initial_sync_done();

    bool datetime_changed = !s_topbar_cache.valid || info.tm_min != s_topbar_cache.minute ||
                            info.tm_hour != s_topbar_cache.hour || info.tm_mday != s_topbar_cache.day ||
                            info.tm_mon != s_topbar_cache.month || info.tm_year != s_topbar_cache.year;
    bool status_changed = !s_topbar_cache.valid || wifi_connected != s_topbar_cache.wifi_connected ||
                          wifi_setup_ap_active != s_topbar_cache.wifi_setup_ap_active ||
                          ha_connected != s_topbar_cache.ha_connected ||
                          ha_initial_sync_done != s_topbar_cache.ha_initial_sync_done;

    if (datetime_changed) {
        ui_pages_set_topbar_datetime(&info);
    }
    if (status_changed) {
        ui_pages_set_topbar_status(wifi_connected, wifi_setup_ap_active, ha_connected, ha_initial_sync_done);
    }

    s_topbar_cache.valid = true;
    s_topbar_cache.minute = info.tm_min;
    s_topbar_cache.hour = info.tm_hour;
    s_topbar_cache.day = info.tm_mday;
    s_topbar_cache.month = info.tm_mon;
    s_topbar_cache.year = info.tm_year;
    s_topbar_cache.wifi_connected = wifi_connected;
    s_topbar_cache.wifi_setup_ap_active = wifi_setup_ap_active;
    s_topbar_cache.ha_connected = ha_connected;
    s_topbar_cache.ha_initial_sync_done = ha_initial_sync_done;
}

static void ui_runtime_show_weather_icon_overlay(void)
{
#if APP_UI_TEST_WEATHER_ICON_OVERLAY
    if (s_weather_icon_overlay != NULL) {
        lv_obj_move_foreground(s_weather_icon_overlay);
        return;
    }

    const lv_font_t *font = mdi_font_weather();
    if (font == NULL) {
        font = mdi_font_large();
    }

    s_weather_icon_overlay = lv_label_create(lv_layer_top());
    lv_obj_add_flag(s_weather_icon_overlay, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_style_text_color(s_weather_icon_overlay, lv_color_hex(0x2FE3E3), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_weather_icon_overlay, LV_OPA_TRANSP, LV_PART_MAIN);
    if (font != NULL) {
        lv_obj_set_style_text_font(s_weather_icon_overlay, font, LV_PART_MAIN);
    }

    /* Rainy icon U+F0597 rendered directly from weather font on top layer. */
    lv_label_set_text(s_weather_icon_overlay, "\xF3\xB0\x96\x97");
    lv_obj_align(s_weather_icon_overlay, LV_ALIGN_CENTER, 0, -20);
    lv_obj_move_foreground(s_weather_icon_overlay);

    ESP_LOGI(TAG_UI, "Weather icon overlay test enabled (font=%s)",
        (mdi_font_weather() != NULL) ? "72/56" : "none");
#endif
}

static void ui_runtime_apply_entity_state_ex(const char *entity_id, bool mark_unavailable_if_missing)
{
    if (entity_id == NULL || entity_id[0] == '\0') {
        return;
    }

    ha_state_t state = {0};
    bool found = ha_model_get_state(entity_id, &state);
    for (size_t i = 0; i < s_widget_count; i++) {
        bool is_primary = (strncmp(entity_id, s_widgets[i].entity_id, APP_MAX_ENTITY_ID_LEN) == 0);
        bool is_secondary = (s_widgets[i].secondary_entity_id[0] != '\0') &&
                            (strncmp(entity_id, s_widgets[i].secondary_entity_id, APP_MAX_ENTITY_ID_LEN) == 0);
        if (!is_primary && !is_secondary) {
            continue;
        }
        if (found) {
            ui_widget_factory_apply_state(&s_widgets[i], &state);
        } else if (is_primary && mark_unavailable_if_missing) {
            ui_widget_factory_mark_unavailable(&s_widgets[i]);
        }
    }
}

static void ui_runtime_apply_entity_state(const char *entity_id)
{
    ui_runtime_apply_entity_state_ex(entity_id, true);
}

static void ui_runtime_apply_all_states(void)
{
    for (size_t i = 0; i < s_widget_count; i++) {
        ui_runtime_apply_entity_state(s_widgets[i].entity_id);
        if (s_widgets[i].secondary_entity_id[0] != '\0' &&
            strncmp(s_widgets[i].secondary_entity_id, s_widgets[i].entity_id, APP_MAX_ENTITY_ID_LEN) != 0) {
            ui_runtime_apply_entity_state(s_widgets[i].secondary_entity_id);
        }
    }
}

static void ui_runtime_apply_all_states_preserve_missing(void)
{
    for (size_t i = 0; i < s_widget_count; i++) {
        ui_runtime_apply_entity_state_ex(s_widgets[i].entity_id, false);
        if (s_widgets[i].secondary_entity_id[0] != '\0' &&
            strncmp(s_widgets[i].secondary_entity_id, s_widgets[i].entity_id, APP_MAX_ENTITY_ID_LEN) != 0) {
            ui_runtime_apply_entity_state_ex(s_widgets[i].secondary_entity_id, false);
        }
    }
}

static bool ui_runtime_widget_from_json(cJSON *widget_json, ui_widget_def_t *out)
{
    cJSON *id = cJSON_GetObjectItemCaseSensitive(widget_json, "id");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(widget_json, "type");
    cJSON *title = cJSON_GetObjectItemCaseSensitive(widget_json, "title");
    cJSON *entity_id = cJSON_GetObjectItemCaseSensitive(widget_json, "entity_id");
    cJSON *secondary_entity_id = cJSON_GetObjectItemCaseSensitive(widget_json, "secondary_entity_id");
    cJSON *slider_direction = cJSON_GetObjectItemCaseSensitive(widget_json, "slider_direction");
    cJSON *slider_accent_color = cJSON_GetObjectItemCaseSensitive(widget_json, "slider_accent_color");
    cJSON *button_accent_color = cJSON_GetObjectItemCaseSensitive(widget_json, "button_accent_color");
    cJSON *button_mode = cJSON_GetObjectItemCaseSensitive(widget_json, "button_mode");
    cJSON *graph_line_color = cJSON_GetObjectItemCaseSensitive(widget_json, "graph_line_color");
    cJSON *graph_point_count = cJSON_GetObjectItemCaseSensitive(widget_json, "graph_point_count");
    cJSON *graph_time_window_min = cJSON_GetObjectItemCaseSensitive(widget_json, "graph_time_window_min");
    cJSON *rect = cJSON_GetObjectItemCaseSensitive(widget_json, "rect");
    if (!cJSON_IsString(id) || !cJSON_IsString(type) || !cJSON_IsObject(rect)) {
        return false;
    }

    const bool requires_entity = (strcmp(type->valuestring, "empty_tile") != 0);
    if (requires_entity && !cJSON_IsString(entity_id)) {
        return false;
    }

    cJSON *x = cJSON_GetObjectItemCaseSensitive(rect, "x");
    cJSON *y = cJSON_GetObjectItemCaseSensitive(rect, "y");
    cJSON *w = cJSON_GetObjectItemCaseSensitive(rect, "w");
    cJSON *h = cJSON_GetObjectItemCaseSensitive(rect, "h");
    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y) || !cJSON_IsNumber(w) || !cJSON_IsNumber(h)) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    snprintf(out->id, sizeof(out->id), "%s", id->valuestring);
    snprintf(out->type, sizeof(out->type), "%s", type->valuestring);
    snprintf(out->title, sizeof(out->title), "%s", cJSON_IsString(title) ? title->valuestring : id->valuestring);
    if (cJSON_IsString(entity_id) && entity_id->valuestring != NULL) {
        snprintf(out->entity_id, sizeof(out->entity_id), "%s", entity_id->valuestring);
    }
    if (cJSON_IsString(secondary_entity_id) && secondary_entity_id->valuestring != NULL) {
        snprintf(out->secondary_entity_id, sizeof(out->secondary_entity_id), "%s", secondary_entity_id->valuestring);
    }
    if (cJSON_IsString(slider_direction) && slider_direction->valuestring != NULL) {
        snprintf(out->slider_direction, sizeof(out->slider_direction), "%s", slider_direction->valuestring);
    }
    if (cJSON_IsString(slider_accent_color) && slider_accent_color->valuestring != NULL) {
        snprintf(out->slider_accent_color, sizeof(out->slider_accent_color), "%s", slider_accent_color->valuestring);
    }
    if (cJSON_IsString(button_accent_color) && button_accent_color->valuestring != NULL) {
        snprintf(out->button_accent_color, sizeof(out->button_accent_color), "%s", button_accent_color->valuestring);
    }
    if (cJSON_IsString(button_mode) && button_mode->valuestring != NULL) {
        snprintf(out->button_mode, sizeof(out->button_mode), "%s", button_mode->valuestring);
    }
    if (cJSON_IsString(graph_line_color) && graph_line_color->valuestring != NULL) {
        snprintf(out->graph_line_color, sizeof(out->graph_line_color), "%s", graph_line_color->valuestring);
    }
    if (cJSON_IsNumber(graph_point_count)) {
        out->graph_point_count = graph_point_count->valueint;
    }
    if (cJSON_IsNumber(graph_time_window_min)) {
        out->graph_time_window_min = graph_time_window_min->valueint;
    }
    out->x = x->valueint;
    out->y = y->valueint;
    out->w = w->valueint;
    out->h = h->valueint;
    ui_runtime_clamp_widget_rect(out);
    return true;
}

static bool ui_runtime_is_background_widget_type(const char *type)
{
    return type != NULL && strcmp(type, "empty_tile") == 0;
}

esp_err_t ui_runtime_load_layout(const char *layout_json)
{
    if (!s_initialized || layout_json == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    cJSON *root = cJSON_Parse(layout_json);
    if (root == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    cJSON *pages = cJSON_GetObjectItemCaseSensitive(root, "pages");
    if (!cJSON_IsArray(pages)) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    if (!display_lock(0)) {
        cJSON_Delete(root);
        return ESP_ERR_TIMEOUT;
    }

    s_topbar_cache.valid = false;
    ui_pages_reset();
    memset(s_widgets, 0, sizeof(s_widgets));
    s_widget_count = 0;

    int page_count = cJSON_GetArraySize(pages);
    for (int p = 0; p < page_count; p++) {
        cJSON *page = cJSON_GetArrayItem(pages, p);
        cJSON *page_id = cJSON_GetObjectItemCaseSensitive(page, "id");
        cJSON *page_title = cJSON_GetObjectItemCaseSensitive(page, "title");
        cJSON *widgets = cJSON_GetObjectItemCaseSensitive(page, "widgets");
        if (!cJSON_IsString(page_id) || !cJSON_IsArray(widgets)) {
            continue;
        }

        lv_obj_t *page_container = ui_pages_add(
            page_id->valuestring, cJSON_IsString(page_title) ? page_title->valuestring : page_id->valuestring);
        if (page_container == NULL) {
            continue;
        }

        int widget_count = cJSON_GetArraySize(widgets);
        for (int pass = 0; pass < 2; pass++) {
            const bool background_pass = (pass == 0);
            for (int w = 0; w < widget_count; w++) {
                if (s_widget_count >= APP_MAX_WIDGETS_TOTAL) {
                    break;
                }
                ui_widget_def_t def = {0};
                if (!ui_runtime_widget_from_json(cJSON_GetArrayItem(widgets, w), &def)) {
                    continue;
                }
                bool is_background = ui_runtime_is_background_widget_type(def.type);
                if (is_background != background_pass) {
                    continue;
                }
                esp_err_t err = ui_widget_factory_create(&def, page_container, &s_widgets[s_widget_count]);
                if (err == ESP_OK) {
                    s_widget_count++;
                }
            }
        }
    }

    cJSON_Delete(root);

    if (ui_pages_count() > 0) {
        ui_pages_show_index(0);
    }
    ui_runtime_apply_all_states();
    ui_runtime_refresh_topbar();
    display_unlock();
    ESP_LOGI(TAG_UI, "Layout loaded: %u widgets", (unsigned)s_widget_count);
    return ESP_OK;
}

esp_err_t ui_runtime_reload_layout(void)
{
    char *json = NULL;
    esp_err_t err = layout_store_load(&json);
    if (err != ESP_OK || json == NULL) {
        json = strdup(layout_store_default_json());
        if (json == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    err = ui_runtime_load_layout(json);
    free(json);
    return err;
}

static void ui_runtime_handle_event(const app_event_t *event)
{
    if (event == NULL) {
        return;
    }

    bool needs_lock = (event->type != EV_LAYOUT_UPDATED);
    if (needs_lock && !display_lock(0)) {
        if (event->type == EV_HA_STATE_CHANGED || event->type == EV_HA_CONNECTED) {
            s_pending_state_reconcile = true;
            s_pending_topbar_refresh = true;
        } else if (event->type == EV_HA_DISCONNECTED) {
            s_pending_topbar_refresh = true;
        }

        s_deferred_event_count++;
        int64_t now_ms = esp_timer_get_time() / 1000;
        if ((now_ms - s_deferred_event_log_ms) >= 5000) {
            ESP_LOGW(TAG_UI, "Deferred UI event processing due to display lock contention (deferred=%u)",
                (unsigned)s_deferred_event_count);
            s_deferred_event_log_ms = now_ms;
            s_deferred_event_count = 0;
        }
        return;
    }

    switch (event->type) {
    case EV_HA_STATE_CHANGED:
        ui_runtime_apply_entity_state(event->data.ha_state_changed.entity_id);
#if APP_HA_ROUTE_TRACE_LOG
        ESP_LOGI(TAG_UI, "route panel->ui entity=%s", event->data.ha_state_changed.entity_id);
#endif
        break;
    case EV_HA_CONNECTED:
        ui_runtime_refresh_topbar();
        /* During initial/partial HA sync we may temporarily miss some entities.
         * Preserve currently rendered widgets instead of forcing unavailable. */
        ui_runtime_apply_all_states_preserve_missing();
        break;
    case EV_HA_DISCONNECTED:
        ui_runtime_refresh_topbar();
        break;
    case EV_LAYOUT_UPDATED:
        ui_runtime_reload_layout();
        break;
    case EV_UI_NAVIGATE:
        ui_pages_show(event->data.navigate.page_id);
        break;
    case EV_NONE:
    default:
        break;
    }

    if (needs_lock) {
        display_unlock();
    }
}

static void ui_runtime_task(void *arg)
{
    (void)arg;
    while (true) {
        app_event_t event = {0};
        while (app_events_receive(&event, 0)) {
            ui_runtime_handle_event(&event);
        }

        int64_t now_ms = esp_timer_get_time() / 1000;
        uint32_t model_revision = ha_model_state_revision();
        if (model_revision != s_last_model_revision) {
            s_last_model_revision = model_revision;
            s_model_reconcile_pending = true;
        }

        if ((s_pending_state_reconcile || s_pending_topbar_refresh) && display_lock(20)) {
            if (s_pending_topbar_refresh) {
                ui_runtime_refresh_topbar();
                s_pending_topbar_refresh = false;
            }
            if (s_pending_state_reconcile) {
                /* Reconcile all states after lock contention so UI never stays stale. */
                ui_runtime_apply_all_states_preserve_missing();
                s_pending_state_reconcile = false;
            }
            display_unlock();
        }

        if (s_model_reconcile_pending && (now_ms - s_last_model_reconcile_ms) >= UI_MODEL_RECONCILE_INTERVAL_MS &&
            display_lock(20)) {
            /* Protect against missed per-entity events under burst: periodically reconcile from model snapshot. */
            ui_runtime_apply_all_states_preserve_missing();
            display_unlock();
            s_model_reconcile_pending = false;
            s_last_model_reconcile_ms = now_ms;
        }

        if ((now_ms - s_last_topbar_refresh_ms) >= 1000) {
            if (display_lock(20)) {
                ui_runtime_refresh_topbar();
                display_unlock();
                s_last_topbar_refresh_ms = now_ms;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t ui_runtime_init(void)
{
    if (!display_lock(0)) {
        return ESP_ERR_TIMEOUT;
    }
    s_topbar_cache.valid = false;
    theme_default_init();
    ui_pages_init();
    ui_runtime_show_weather_icon_overlay();
    ui_runtime_refresh_topbar();
    display_unlock();
    s_last_topbar_refresh_ms = esp_timer_get_time() / 1000;
    s_last_model_reconcile_ms = s_last_topbar_refresh_ms;
    s_last_model_revision = ha_model_state_revision();
    s_model_reconcile_pending = false;
    s_pending_state_reconcile = false;
    s_pending_topbar_refresh = false;
    s_deferred_event_count = 0;
    s_deferred_event_log_ms = 0;
    s_initialized = true;
    return ESP_OK;
}

esp_err_t ui_runtime_start(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_ui_task != NULL) {
        return ESP_OK;
    }

    BaseType_t created =
        xTaskCreate(ui_runtime_task, "ui_runtime", APP_UI_TASK_STACK, NULL, APP_UI_TASK_PRIO, &s_ui_task);
    return (created == pdPASS) ? ESP_OK : ESP_FAIL;
}
