#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_timer.h"

#include "ui/fonts/app_text_fonts.h"
#include "ui/ui_i18n.h"
#include "ui/theme/theme_default.h"

#if LV_FONT_MONTSERRAT_24
#define SENSOR_VALUE_FONT_SMALL APP_FONT_TEXT_24
#elif LV_FONT_MONTSERRAT_22
#define SENSOR_VALUE_FONT_SMALL APP_FONT_TEXT_22
#elif LV_FONT_MONTSERRAT_20
#define SENSOR_VALUE_FONT_SMALL APP_FONT_TEXT_20
#else
#define SENSOR_VALUE_FONT_SMALL APP_FONT_TEXT_20
#endif

#if LV_FONT_MONTSERRAT_32
#define SENSOR_VALUE_FONT_MEDIUM (&lv_font_montserrat_32)
#elif LV_FONT_MONTSERRAT_28
#define SENSOR_VALUE_FONT_MEDIUM APP_FONT_TEXT_28
#elif LV_FONT_MONTSERRAT_24
#define SENSOR_VALUE_FONT_MEDIUM APP_FONT_TEXT_24
#else
#define SENSOR_VALUE_FONT_MEDIUM SENSOR_VALUE_FONT_SMALL
#endif

#if LV_FONT_MONTSERRAT_44
#define SENSOR_VALUE_FONT_LARGE (&lv_font_montserrat_44)
#elif LV_FONT_MONTSERRAT_40
#define SENSOR_VALUE_FONT_LARGE (&lv_font_montserrat_40)
#elif LV_FONT_MONTSERRAT_36
#define SENSOR_VALUE_FONT_LARGE (&lv_font_montserrat_36)
#elif LV_FONT_MONTSERRAT_34
#define SENSOR_VALUE_FONT_LARGE APP_FONT_TEXT_34
#elif LV_FONT_MONTSERRAT_32
#define SENSOR_VALUE_FONT_LARGE (&lv_font_montserrat_32)
#else
#define SENSOR_VALUE_FONT_LARGE SENSOR_VALUE_FONT_MEDIUM
#endif

#if LV_FONT_MONTSERRAT_56
#define SENSOR_VALUE_FONT_XL (&lv_font_montserrat_56)
#elif LV_FONT_MONTSERRAT_52
#define SENSOR_VALUE_FONT_XL (&lv_font_montserrat_52)
#elif LV_FONT_MONTSERRAT_48
#define SENSOR_VALUE_FONT_XL (&lv_font_montserrat_48)
#elif LV_FONT_MONTSERRAT_44
#define SENSOR_VALUE_FONT_XL (&lv_font_montserrat_44)
#else
#define SENSOR_VALUE_FONT_XL SENSOR_VALUE_FONT_LARGE
#endif

#if LV_FONT_MONTSERRAT_18
#define SENSOR_META_FONT (&lv_font_montserrat_18)
#elif LV_FONT_MONTSERRAT_16
#define SENSOR_META_FONT (&lv_font_montserrat_16)
#else
#define SENSOR_META_FONT APP_FONT_TEXT_14
#endif

typedef struct {
    lv_obj_t *card;
    lv_obj_t *title_label;
    lv_obj_t *value_label;
    lv_obj_t *age_label;
    int64_t last_update_ms;
    bool has_timestamp;
    bool unavailable;
    lv_timer_t *age_timer;
} w_sensor_ctx_t;

static bool sensor_state_is_unavailable(const char *state_text)
{
    if (state_text == NULL || state_text[0] == '\0') {
        return true;
    }
    return strcmp(state_text, "unavailable") == 0 || strcmp(state_text, "unknown") == 0;
}

static int64_t sensor_now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static const lv_font_t *sensor_pick_value_font(const w_sensor_ctx_t *ctx)
{
    if (ctx == NULL || ctx->card == NULL) {
        return SENSOR_VALUE_FONT_MEDIUM;
    }

    lv_coord_t w = lv_obj_get_width(ctx->card);
    lv_coord_t h = lv_obj_get_height(ctx->card);
    lv_coord_t min_dim = (w < h) ? w : h;

    if (min_dim >= 260) {
        return SENSOR_VALUE_FONT_XL;
    }
    if (min_dim >= 190) {
        return SENSOR_VALUE_FONT_LARGE;
    }
    if (min_dim >= 140) {
        return SENSOR_VALUE_FONT_MEDIUM;
    }
    return SENSOR_VALUE_FONT_SMALL;
}

static void sensor_set_value_text(w_sensor_ctx_t *ctx, const char *text)
{
    if (ctx == NULL || ctx->value_label == NULL) {
        return;
    }
    lv_label_set_text(ctx->value_label, (text != NULL && text[0] != '\0') ? text : "--");
}

static void sensor_update_age_label(w_sensor_ctx_t *ctx)
{
    if (ctx == NULL || ctx->age_label == NULL) {
        return;
    }

    if (ctx->unavailable || !ctx->has_timestamp) {
        lv_obj_add_flag(ctx->age_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    int64_t age_ms = sensor_now_ms() - ctx->last_update_ms;
    if (age_ms < 0) {
        age_ms = 0;
    }

    int32_t age_min = (int32_t)(age_ms / 60000);
    int32_t age_hour = age_min / 60;
    int32_t age_day = age_hour / 24;

    char text[32] = {0};
    if (age_min <= 0) {
        snprintf(text, sizeof(text), "%s", ui_i18n_get("sensor.age.just_now", "just now"));
    } else if (age_min == 1) {
        snprintf(text, sizeof(text), "%s", ui_i18n_get("sensor.age.min_one", "1 min ago"));
    } else if (age_min < 60) {
        snprintf(text, sizeof(text), ui_i18n_get("sensor.age.min_many", "%d min ago"), (int)age_min);
    } else if (age_hour == 1) {
        snprintf(text, sizeof(text), "%s", ui_i18n_get("sensor.age.hour_one", "1 hour ago"));
    } else if (age_hour < 48) {
        snprintf(text, sizeof(text), ui_i18n_get("sensor.age.hour_many", "%d hours ago"), (int)age_hour);
    } else if (age_day == 1) {
        snprintf(text, sizeof(text), "%s", ui_i18n_get("sensor.age.day_one", "1 day ago"));
    } else {
        snprintf(text, sizeof(text), ui_i18n_get("sensor.age.day_many", "%d days ago"), (int)age_day);
    }

    lv_label_set_text(ctx->age_label, text);
    lv_obj_clear_flag(ctx->age_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(
        ctx->age_label,
        lv_color_hex((age_min >= 30) ? APP_UI_COLOR_STATE_OFF : APP_UI_COLOR_TEXT_MUTED),
        LV_PART_MAIN);
}

static void sensor_apply_layout(w_sensor_ctx_t *ctx)
{
    if (ctx == NULL || ctx->card == NULL || ctx->title_label == NULL || ctx->value_label == NULL || ctx->age_label == NULL) {
        return;
    }

    lv_obj_t *card = ctx->card;
    lv_obj_update_layout(card);

    lv_coord_t content_w =
        lv_obj_get_width(card) - lv_obj_get_style_pad_left(card, LV_PART_MAIN) - lv_obj_get_style_pad_right(card, LV_PART_MAIN);
    lv_coord_t content_h =
        lv_obj_get_height(card) - lv_obj_get_style_pad_top(card, LV_PART_MAIN) - lv_obj_get_style_pad_bottom(card, LV_PART_MAIN);
    if (content_w < 24) {
        content_w = 24;
    }
    if (content_h < 40) {
        content_h = 40;
    }

    lv_obj_set_style_text_font(ctx->value_label, sensor_pick_value_font(ctx), LV_PART_MAIN);
    lv_obj_set_style_text_font(ctx->age_label, SENSOR_META_FONT, LV_PART_MAIN);

    lv_obj_set_width(ctx->title_label, content_w);
    lv_obj_set_width(ctx->value_label, content_w);
    lv_obj_set_width(ctx->age_label, content_w);
    lv_obj_set_style_text_align(ctx->title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_align(ctx->value_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_align(ctx->age_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_align(ctx->title_label, LV_ALIGN_TOP_MID, 0, APP_UI_TILE_LAYOUT_TUNED ? 2 : 0);

    const bool show_age = !lv_obj_has_flag(ctx->age_label, LV_OBJ_FLAG_HIDDEN);
    lv_coord_t min_dim = (content_w < content_h) ? content_w : content_h;
    lv_coord_t value_y = 0;
    if (show_age) {
        if (min_dim >= 260) {
            value_y = -18;
        } else if (min_dim >= 190) {
            value_y = -14;
        } else {
            value_y = -10;
        }
    }
    lv_obj_align(ctx->value_label, LV_ALIGN_CENTER, 0, value_y);

    if (show_age) {
        lv_obj_align_to(ctx->age_label, ctx->value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
        lv_obj_update_layout(ctx->age_label);
        lv_coord_t age_bottom = lv_obj_get_y(ctx->age_label) + lv_obj_get_height(ctx->age_label);
        if (age_bottom > content_h - 2) {
            lv_obj_set_y(ctx->age_label, content_h - lv_obj_get_height(ctx->age_label) - 2);
        }
    }
}

static void sensor_apply_unavailable(w_sensor_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ctx->unavailable = true;
    ctx->has_timestamp = false;
    ctx->last_update_ms = 0;
    sensor_set_value_text(ctx, ui_i18n_get("common.unavailable", "unavailable"));
    sensor_update_age_label(ctx);
    sensor_apply_layout(ctx);
}

static void sensor_age_timer_cb(lv_timer_t *timer)
{
    if (timer == NULL) {
        return;
    }
    w_sensor_ctx_t *ctx = (w_sensor_ctx_t *)lv_timer_get_user_data(timer);
    if (ctx == NULL) {
        return;
    }

    sensor_update_age_label(ctx);
    sensor_apply_layout(ctx);
}

static void w_sensor_event_cb(lv_event_t *event)
{
    if (event == NULL) {
        return;
    }

    w_sensor_ctx_t *ctx = (w_sensor_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_DELETE) {
        if (ctx->age_timer != NULL) {
            lv_timer_del(ctx->age_timer);
            ctx->age_timer = NULL;
        }
        free(ctx);
    } else if (code == LV_EVENT_SIZE_CHANGED) {
        sensor_apply_layout(ctx);
    }
}

esp_err_t w_sensor_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
{
    if (def == NULL || parent == NULL || out_instance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, def->x, def->y);
    lv_obj_set_size(card, def->w, def->h);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    theme_default_style_card(card);
    lv_obj_set_style_pad_left(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_top(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(card, 10, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->id);
    lv_obj_set_style_text_color(title, theme_default_color_text_muted(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, APP_FONT_TEXT_20, LV_PART_MAIN);

    lv_obj_t *value = lv_label_create(card);
    lv_label_set_text(value, "--");
    lv_obj_set_style_text_color(value, theme_default_color_text_primary(), LV_PART_MAIN);
    lv_obj_set_style_text_font(value, SENSOR_VALUE_FONT_MEDIUM, LV_PART_MAIN);

    lv_obj_t *age = lv_label_create(card);
    lv_label_set_text(age, ui_i18n_get("sensor.age.just_now", "just now"));
    lv_obj_set_style_text_color(age, theme_default_color_text_muted(), LV_PART_MAIN);
    lv_obj_set_style_text_font(age, SENSOR_META_FONT, LV_PART_MAIN);
    lv_obj_add_flag(age, LV_OBJ_FLAG_HIDDEN);

    w_sensor_ctx_t *ctx = calloc(1, sizeof(w_sensor_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }

    ctx->card = card;
    ctx->title_label = title;
    ctx->value_label = value;
    ctx->age_label = age;
    ctx->last_update_ms = 0;
    ctx->has_timestamp = false;
    ctx->unavailable = false;
    ctx->age_timer = lv_timer_create(sensor_age_timer_cb, 30000, ctx);

    lv_obj_add_event_cb(card, w_sensor_event_cb, LV_EVENT_DELETE, ctx);
    lv_obj_add_event_cb(card, w_sensor_event_cb, LV_EVENT_SIZE_CHANGED, ctx);

    sensor_update_age_label(ctx);
    sensor_apply_layout(ctx);

    out_instance->obj = card;
    out_instance->ctx = ctx;
    return ESP_OK;
}

void w_sensor_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }

    w_sensor_ctx_t *ctx = (w_sensor_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }

    if (sensor_state_is_unavailable(state->state)) {
        sensor_apply_unavailable(ctx);
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

    ctx->unavailable = false;
    ctx->last_update_ms = state->last_changed_unix_ms;
    ctx->has_timestamp = ctx->last_update_ms > 0;

    sensor_set_value_text(ctx, value_text);
    sensor_update_age_label(ctx);
    sensor_apply_layout(ctx);
}

void w_sensor_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }

    w_sensor_ctx_t *ctx = (w_sensor_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }

    sensor_apply_unavailable(ctx);
}
