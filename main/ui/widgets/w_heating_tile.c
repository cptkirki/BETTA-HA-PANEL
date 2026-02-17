#include "ui/ui_widget_factory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "ui/ui_bindings.h"
#include "ui/ui_i18n.h"
#include "ui/theme/theme_default.h"

#if LV_FONT_MONTSERRAT_34
#define HEATING_ACTUAL_FONT (&lv_font_montserrat_34)
#elif LV_FONT_MONTSERRAT_32
#define HEATING_ACTUAL_FONT (&lv_font_montserrat_32)
#elif LV_FONT_MONTSERRAT_28
#define HEATING_ACTUAL_FONT (&lv_font_montserrat_28)
#elif LV_FONT_MONTSERRAT_24
#define HEATING_ACTUAL_FONT (&lv_font_montserrat_24)
#else
#define HEATING_ACTUAL_FONT LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_20
#define HEATING_TARGET_FONT (&lv_font_montserrat_20)
#elif LV_FONT_MONTSERRAT_18
#define HEATING_TARGET_FONT (&lv_font_montserrat_18)
#elif LV_FONT_MONTSERRAT_16
#define HEATING_TARGET_FONT (&lv_font_montserrat_16)
#else
#define HEATING_TARGET_FONT LV_FONT_DEFAULT
#endif

typedef struct {
    char climate_entity_id[APP_MAX_ENTITY_ID_LEN];
    char sensor_entity_id[APP_MAX_ENTITY_ID_LEN];
    bool is_on;
    float target_temp;
    float current_temp;
    bool has_current_temp;
    char status_text[32];
    lv_obj_t *icon_label;
    lv_obj_t *title_label;
    lv_obj_t *arc;
    lv_obj_t *target_label;
    lv_obj_t *actual_label;
    lv_obj_t *status_label;
} w_heating_tile_ctx_t;

typedef struct {
    float target_temp;
    float current_temp;
    bool has_current_temp;
    char status_text[32];
} heating_values_t;

#define HEATING_ICON_SYMBOL_CP 0xF011U
#define HEATING_ARC_SIZE_MIN 140
#define HEATING_ARC_SIZE_MAX 340

static bool heating_icon_symbol_available(void)
{
    static bool checked = false;
    static bool available = false;

    if (!checked) {
        checked = true;
        lv_font_glyph_dsc_t dsc = {0};
        available = lv_font_get_glyph_dsc(LV_FONT_DEFAULT, &dsc, HEATING_ICON_SYMBOL_CP, 0);
    }

    return available;
}

static const char *heating_icon_text(void)
{
    return heating_icon_symbol_available() ? LV_SYMBOL_POWER : "H";
}

static bool heating_parse_float_relaxed(const char *text, float *out_value)
{
    if (text == NULL || out_value == NULL || text[0] == '\0') {
        return false;
    }

    char buf[32] = {0};
    size_t n = strnlen(text, sizeof(buf) - 1U);
    for (size_t i = 0; i < n; i++) {
        buf[i] = (text[i] == ',') ? '.' : text[i];
    }
    buf[n] = '\0';

    char *end = NULL;
    float parsed = strtof(buf, &end);
    if (end == buf) {
        return false;
    }
    *out_value = parsed;
    return true;
}

static float clamp_temp(float value)
{
    if (value < 5.0f) {
        return 5.0f;
    }
    if (value > 30.0f) {
        return 30.0f;
    }
    return value;
}

static void heating_copy_text(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    snprintf(dst, dst_size, "%.*s", (int)(dst_size - 1U), src);
}

static bool heating_state_is_on(const char *state)
{
    if (state == NULL || state[0] == '\0') {
        return false;
    }
    if (strcmp(state, "off") == 0 || strcmp(state, "unavailable") == 0 || strcmp(state, "unknown") == 0) {
        return false;
    }
    return true;
}

static void heating_extract_climate_values(const ha_state_t *state, heating_values_t *out)
{
    if (state == NULL || out == NULL) {
        return;
    }

    out->target_temp = 20.0f;
    out->current_temp = 20.0f;
    out->has_current_temp = false;
    out->status_text[0] = '\0';
    bool has_target_temp = false;

    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *temperature = cJSON_GetObjectItemCaseSensitive(attrs, "temperature");
        if (cJSON_IsNumber(temperature)) {
            out->target_temp = (float)temperature->valuedouble;
            has_target_temp = true;
        }

        cJSON *current_temperature = cJSON_GetObjectItemCaseSensitive(attrs, "current_temperature");
        if (cJSON_IsNumber(current_temperature)) {
            out->current_temp = (float)current_temperature->valuedouble;
            out->has_current_temp = true;
        }

        cJSON *hvac_action = cJSON_GetObjectItemCaseSensitive(attrs, "hvac_action");
        if (cJSON_IsString(hvac_action) && hvac_action->valuestring != NULL && hvac_action->valuestring[0] != '\0') {
            heating_copy_text(out->status_text, sizeof(out->status_text), hvac_action->valuestring);
        }
        cJSON_Delete(attrs);
    }

    if (!has_target_temp) {
        char *end = NULL;
        float parsed = strtof(state->state, &end);
        if (end != state->state) {
            out->target_temp = parsed;
        }
    }

    out->target_temp = clamp_temp(out->target_temp);
    out->current_temp = clamp_temp(out->current_temp);

    if (out->status_text[0] == '\0' && state->state[0] != '\0') {
        heating_copy_text(out->status_text, sizeof(out->status_text), state->state);
    }
}

static bool heating_extract_sensor_temp(const ha_state_t *state, float *out_temp)
{
    if (state == NULL || out_temp == NULL) {
        return false;
    }

    float parsed = 0.0f;
    if (heating_parse_float_relaxed(state->state, &parsed)) {
        *out_temp = clamp_temp(parsed);
        return true;
    }

    cJSON *attrs = cJSON_Parse(state->attributes_json);
    if (attrs != NULL) {
        cJSON *value = cJSON_GetObjectItemCaseSensitive(attrs, "temperature");
        if (cJSON_IsNumber(value)) {
            *out_temp = clamp_temp((float)value->valuedouble);
            cJSON_Delete(attrs);
            return true;
        }
        cJSON_Delete(attrs);
    }

    return false;
}

static void heating_set_target_label(lv_obj_t *label, float value)
{
    if (label == NULL) {
        return;
    }
    char text[20] = {0};
    snprintf(text, sizeof(text), ui_i18n_get("heating.target_format", "Target %.1f C"), (double)clamp_temp(value));
    lv_label_set_text(label, text);
}

static void heating_set_actual_label(lv_obj_t *label, bool has_current_temp, float current_temp, const char *status_text)
{
    if (label == NULL) {
        return;
    }

    if (has_current_temp) {
        char text[20] = {0};
        snprintf(text, sizeof(text), "%.1f C", (double)clamp_temp(current_temp));
        lv_label_set_text(label, text);
        return;
    }
    if (status_text != NULL && status_text[0] != '\0') {
        lv_label_set_text(label, status_text);
    } else {
        lv_label_set_text(label, "--.- C");
    }
}

static void heating_set_status_label(lv_obj_t *label, bool is_on, const char *status_text)
{
    if (label == NULL) {
        return;
    }

    if (status_text != NULL && status_text[0] != '\0') {
        char text[40] = {0};
        size_t out = 0;
        for (size_t i = 0; status_text[i] != '\0' && out + 1 < sizeof(text); i++) {
            char c = status_text[i];
            if (c == '_') {
                c = ' ';
            }
            if (c >= 'A' && c <= 'Z') {
                c = (char)(c + ('a' - 'A'));
            }
            text[out++] = c;
        }
        text[out] = '\0';
        if (strcmp(text, "on") == 0) {
            lv_label_set_text(label, ui_i18n_get("common.on", "ON"));
        } else if (strcmp(text, "off") == 0) {
            lv_label_set_text(label, ui_i18n_get("common.off", "OFF"));
        } else if (strcmp(text, "unavailable") == 0) {
            lv_label_set_text(label, ui_i18n_get("common.unavailable", "unavailable"));
        } else if (strcmp(text, "heating") == 0) {
            lv_label_set_text(label, ui_i18n_get("heating.active", "heating active"));
        } else {
            lv_label_set_text(label, text);
        }
        return;
    }

    lv_label_set_text(
        label, is_on ? ui_i18n_get("heating.active", "heating active") : ui_i18n_get("common.off", "OFF"));
}

static void heating_apply_layout(lv_obj_t *card, w_heating_tile_ctx_t *ctx)
{
    if (card == NULL || ctx == NULL || ctx->arc == NULL || ctx->target_label == NULL ||
        ctx->actual_label == NULL || ctx->status_label == NULL) {
        return;
    }

    int card_w = lv_obj_get_width(card);
    int card_h = lv_obj_get_height(card);

    /* Keep arc fully inside the card to avoid expensive clipping/mask paths on rounded tiles. */
    int arc_size = card_w - 38;
    int max_arc_h = card_h - 70;
    if (arc_size > max_arc_h) {
        arc_size = max_arc_h;
    }
    if (arc_size < HEATING_ARC_SIZE_MIN) {
        arc_size = HEATING_ARC_SIZE_MIN;
    }
    if (arc_size > HEATING_ARC_SIZE_MAX) {
        arc_size = HEATING_ARC_SIZE_MAX;
    }

    int arc_y = 16;
    if (card_h >= 320) {
        arc_y = 22;
    } else if (card_h >= 280) {
        arc_y = 18;
    }

    /* Clamp Y offset so the arc never spills out of the card. */
    int arc_y_max = (card_h / 2) - 8 - (arc_size / 2);
    if (arc_y > arc_y_max) {
        arc_y = arc_y_max;
    }
    if (arc_y < -arc_y_max) {
        arc_y = -arc_y_max;
    }

    lv_obj_set_size(ctx->arc, arc_size, arc_size);
    lv_obj_align(ctx->arc, LV_ALIGN_CENTER, 0, arc_y);

    int center_y = (card_h / 2) + arc_y;
    int target_y = center_y + ((card_h >= 300) ? 8 : 6);
    int actual_y = target_y + ((card_h >= 300) ? 46 : 40);
    int status_y = card_h - 34;

    if (actual_y > (status_y - 30)) {
        actual_y = status_y - 30;
    }
    if (target_y > (actual_y - 36)) {
        target_y = actual_y - 36;
    }
    if (target_y < 90) {
        target_y = 90;
    }

    lv_obj_align(ctx->target_label, LV_ALIGN_TOP_MID, 0, target_y);
    lv_obj_align(ctx->actual_label, LV_ALIGN_TOP_MID, 0, actual_y);
    lv_obj_align(ctx->status_label, LV_ALIGN_TOP_MID, 0, status_y);
}

static void heating_apply_visual(lv_obj_t *card, w_heating_tile_ctx_t *ctx, bool allow_status_fallback)
{
    if (card == NULL || ctx == NULL) {
        return;
    }
    lv_obj_t *icon = ctx->icon_label;
    lv_obj_t *title = ctx->title_label;
    lv_obj_t *arc = ctx->arc;
    lv_obj_t *target_label = ctx->target_label;
    lv_obj_t *actual_label = ctx->actual_label;
    lv_obj_t *status_label = ctx->status_label;
    if (icon == NULL || title == NULL || arc == NULL || target_label == NULL || actual_label == NULL || status_label == NULL) {
        return;
    }

    bool is_on = ctx->is_on;
    float target_temp = ctx->target_temp;
    bool has_current_temp = ctx->has_current_temp;
    float current_temp = ctx->current_temp;
    const char *status_text = ctx->status_text;

    lv_obj_set_style_bg_color(
        card, is_on ? lv_color_hex(APP_UI_COLOR_CARD_BG_ON) : lv_color_hex(APP_UI_COLOR_CARD_BG_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(
        icon, is_on ? lv_color_hex(APP_UI_COLOR_HEAT_ICON_ON) : lv_color_hex(APP_UI_COLOR_CARD_ICON_OFF), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(target_label, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
    lv_obj_set_style_text_color(actual_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(status_label, lv_color_hex(APP_UI_COLOR_TEXT_MUTED), LV_PART_MAIN);

    lv_obj_set_style_arc_color(
        arc, is_on ? lv_color_hex(APP_UI_COLOR_HEAT_TRACK_ON) : lv_color_hex(APP_UI_COLOR_HEAT_TRACK_OFF), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, (lv_obj_get_width(card) >= 300) ? 16 : 15, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

    lv_obj_set_style_arc_color(
        arc, is_on ? lv_color_hex(APP_UI_COLOR_HEAT_IND_ON) : lv_color_hex(APP_UI_COLOR_HEAT_IND_OFF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, (lv_obj_get_width(card) >= 300) ? 16 : 15, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);

    lv_obj_set_style_bg_color(
        arc, is_on ? lv_color_hex(APP_UI_COLOR_HEAT_KNOB_ON) : lv_color_hex(APP_UI_COLOR_HEAT_KNOB_OFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc, LV_OPA_COVER, LV_PART_KNOB);

    int arc_value = (int)(clamp_temp(target_temp) + 0.5f);
    lv_arc_set_value(arc, arc_value);
    heating_set_target_label(target_label, target_temp);
    heating_set_actual_label(actual_label, has_current_temp, current_temp, allow_status_fallback ? status_text : "");
    heating_set_status_label(status_label, is_on, status_text);
    heating_apply_layout(card, ctx);
}

static void heating_apply_from_ctx(lv_obj_t *card, const w_heating_tile_ctx_t *ctx)
{
    if (ctx == NULL || card == NULL) {
        return;
    }
    bool allow_status_fallback = (ctx->sensor_entity_id[0] == '\0');
    heating_apply_visual(card, (w_heating_tile_ctx_t *)ctx, allow_status_fallback);
}

static void w_heating_tile_card_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    w_heating_tile_ctx_t *ctx = (w_heating_tile_ctx_t *)lv_event_get_user_data(event);
    if (ctx == NULL) {
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *card = lv_event_get_target(event);
        bool prev_is_on = ctx->is_on;
        char prev_status[sizeof(ctx->status_text)] = {0};
        heating_copy_text(prev_status, sizeof(prev_status), ctx->status_text);

        ctx->is_on = !ctx->is_on;
        heating_copy_text(ctx->status_text, sizeof(ctx->status_text), ctx->is_on ? "on" : "off");
        heating_apply_from_ctx(card, ctx);

        esp_err_t err = ui_bindings_toggle_entity(ctx->climate_entity_id);
        if (err != ESP_OK) {
            ctx->is_on = prev_is_on;
            heating_copy_text(ctx->status_text, sizeof(ctx->status_text), prev_status);
            heating_apply_from_ctx(card, ctx);
        }
    } else if (code == LV_EVENT_DELETE) {
        free(ctx);
    }
}

static void w_heating_tile_arc_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) {
        return;
    }

    lv_obj_t *arc = lv_event_get_target(event);
    w_heating_tile_ctx_t *ctx = (w_heating_tile_ctx_t *)lv_event_get_user_data(event);
    int value = (arc != NULL) ? lv_arc_get_value(arc) : 20;

    heating_set_target_label((ctx != NULL) ? ctx->target_label : NULL, (float)value);

    if (code == LV_EVENT_RELEASED) {
        if (ctx != NULL) {
            ctx->target_temp = (float)value;
            ui_bindings_set_slider_value(ctx->climate_entity_id, value);
        }
    }
}

esp_err_t w_heating_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
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

    lv_obj_t *icon = lv_label_create(card);
    lv_label_set_text(icon, heating_icon_text());
    lv_obj_set_style_text_font(icon, LV_FONT_DEFAULT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 0, 2);
#else
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, def->title[0] ? def->title : def->id);
    lv_obj_set_width(title, def->w - 32);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
#else
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
#endif

    lv_obj_t *arc = lv_arc_create(card);
    lv_obj_set_size(arc, HEATING_ARC_SIZE_MIN, HEATING_ARC_SIZE_MIN);
    lv_arc_set_range(arc, 5, 30);
    lv_arc_set_value(arc, 20);
    lv_arc_set_bg_angles(arc, 160, 20);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 30);
#else
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 20);
#endif
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *target_label = lv_label_create(card);
    heating_set_target_label(target_label, 20.0f);
    lv_obj_set_style_text_font(target_label, HEATING_TARGET_FONT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(target_label, LV_ALIGN_CENTER, 0, 8);
#else
    lv_obj_align(target_label, LV_ALIGN_CENTER, 0, 4);
#endif

    lv_obj_t *actual_label = lv_label_create(card);
    lv_label_set_text(actual_label, "--.- C");
    lv_obj_set_style_text_font(actual_label, HEATING_ACTUAL_FONT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(actual_label, LV_ALIGN_CENTER, 0, 50);
#else
    lv_obj_align(actual_label, LV_ALIGN_CENTER, 0, 40);
#endif

    lv_obj_t *status_label = lv_label_create(card);
    lv_label_set_text(status_label, ui_i18n_get("common.off", "OFF"));
    lv_obj_set_style_text_font(status_label, LV_FONT_DEFAULT, LV_PART_MAIN);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -12);

    w_heating_tile_ctx_t *ctx = calloc(1, sizeof(w_heating_tile_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }
    snprintf(ctx->climate_entity_id, sizeof(ctx->climate_entity_id), "%s", def->entity_id);
    snprintf(ctx->sensor_entity_id, sizeof(ctx->sensor_entity_id), "%s", def->secondary_entity_id);
    ctx->is_on = false;
    ctx->target_temp = 20.0f;
    ctx->current_temp = 20.0f;
    ctx->has_current_temp = false;
    snprintf(ctx->status_text, sizeof(ctx->status_text), "OFF");
    ctx->icon_label = icon;
    ctx->title_label = title;
    ctx->arc = arc;
    ctx->target_label = target_label;
    ctx->actual_label = actual_label;
    ctx->status_label = status_label;

    lv_obj_add_event_cb(card, w_heating_tile_card_event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(card, w_heating_tile_card_event_cb, LV_EVENT_DELETE, ctx);
    lv_obj_add_event_cb(arc, w_heating_tile_arc_event_cb, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(arc, w_heating_tile_arc_event_cb, LV_EVENT_RELEASED, ctx);

    out_instance->ctx = ctx;
    out_instance->obj = card;
    heating_apply_from_ctx(card, ctx);
    return ESP_OK;
}

void w_heating_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }

    w_heating_tile_ctx_t *ctx = (w_heating_tile_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }

    if (strncmp(state->entity_id, ctx->climate_entity_id, APP_MAX_ENTITY_ID_LEN) == 0) {
        heating_values_t values = {0};
        heating_extract_climate_values(state, &values);
        ctx->is_on = heating_state_is_on(state->state);
        ctx->target_temp = values.target_temp;
        heating_copy_text(ctx->status_text, sizeof(ctx->status_text), values.status_text);
        if (ctx->sensor_entity_id[0] == '\0') {
            ctx->has_current_temp = values.has_current_temp;
            ctx->current_temp = values.current_temp;
        }
    } else if (ctx->sensor_entity_id[0] != '\0' &&
               strncmp(state->entity_id, ctx->sensor_entity_id, APP_MAX_ENTITY_ID_LEN) == 0) {
        float sensor_temp = 0.0f;
        bool ok = heating_extract_sensor_temp(state, &sensor_temp);
        ctx->has_current_temp = ok;
        if (ok) {
            ctx->current_temp = sensor_temp;
        }
    } else {
        return;
    }

    heating_apply_from_ctx(instance->obj, ctx);
}

void w_heating_tile_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }

    w_heating_tile_ctx_t *ctx = (w_heating_tile_ctx_t *)instance->ctx;
    if (ctx != NULL) {
        ctx->is_on = false;
        ctx->has_current_temp = false;
        heating_copy_text(ctx->status_text, sizeof(ctx->status_text), "unavailable");
        heating_apply_from_ctx(instance->obj, ctx);
        return;
    }

    w_heating_tile_ctx_t fallback = {0};
    fallback.is_on = false;
    fallback.target_temp = 20.0f;
    fallback.current_temp = 20.0f;
    fallback.has_current_temp = false;
    snprintf(fallback.status_text, sizeof(fallback.status_text), "unavailable");
    fallback.icon_label = lv_obj_get_child(instance->obj, 0);
    fallback.title_label = lv_obj_get_child(instance->obj, 1);
    fallback.arc = lv_obj_get_child(instance->obj, 2);
    fallback.target_label = lv_obj_get_child(instance->obj, 3);
    fallback.actual_label = lv_obj_get_child(instance->obj, 4);
    fallback.status_label = lv_obj_get_child(instance->obj, 5);
    heating_apply_visual(instance->obj, &fallback, true);
}
