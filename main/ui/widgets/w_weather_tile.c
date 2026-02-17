#include "ui/ui_widget_factory.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "cJSON.h"
#include "esp_log.h"

#include "ui/fonts/mdi_font_registry.h"
#include "ui/ui_i18n.h"
#include "ui/theme/theme_default.h"

#ifndef APP_UI_WEATHER_ICON_DEBUG
#define APP_UI_WEATHER_ICON_DEBUG 0
#endif

#ifndef APP_UI_WEATHER_LOTTIE_ASSETS
#define APP_UI_WEATHER_LOTTIE_ASSETS 0
#endif

#if LV_USE_LOTTIE && APP_UI_WEATHER_LOTTIE_ASSETS
#define APP_UI_WEATHER_LOTTIE_ENABLED 1
#else
#define APP_UI_WEATHER_LOTTIE_ENABLED 0
#endif

#if LV_FONT_MONTSERRAT_40
#define WEATHER_TEMP_FONT (&lv_font_montserrat_40)
#elif LV_FONT_MONTSERRAT_38
#define WEATHER_TEMP_FONT (&lv_font_montserrat_38)
#elif LV_FONT_MONTSERRAT_36
#define WEATHER_TEMP_FONT (&lv_font_montserrat_36)
#elif LV_FONT_MONTSERRAT_34
#define WEATHER_TEMP_FONT (&lv_font_montserrat_34)
#elif LV_FONT_MONTSERRAT_32
#define WEATHER_TEMP_FONT (&lv_font_montserrat_32)
#elif LV_FONT_MONTSERRAT_28
#define WEATHER_TEMP_FONT (&lv_font_montserrat_28)
#elif LV_FONT_MONTSERRAT_24
#define WEATHER_TEMP_FONT (&lv_font_montserrat_24)
#else
#define WEATHER_TEMP_FONT LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_48
#define WEATHER_TEMP_FONT_LARGE (&lv_font_montserrat_48)
#elif LV_FONT_MONTSERRAT_44
#define WEATHER_TEMP_FONT_LARGE (&lv_font_montserrat_44)
#else
#define WEATHER_TEMP_FONT_LARGE WEATHER_TEMP_FONT
#endif

#if LV_FONT_MONTSERRAT_20
#define WEATHER_CONDITION_FONT (&lv_font_montserrat_20)
#elif LV_FONT_MONTSERRAT_18
#define WEATHER_CONDITION_FONT (&lv_font_montserrat_18)
#else
#define WEATHER_CONDITION_FONT LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_24
#define WEATHER_META_FONT_LARGE (&lv_font_montserrat_24)
#elif LV_FONT_MONTSERRAT_22
#define WEATHER_META_FONT_LARGE (&lv_font_montserrat_22)
#else
#define WEATHER_META_FONT_LARGE WEATHER_CONDITION_FONT
#endif

#if LV_FONT_MONTSERRAT_32
#define WEATHER_3DAY_TEMP_FONT (&lv_font_montserrat_32)
#elif LV_FONT_MONTSERRAT_28
#define WEATHER_3DAY_TEMP_FONT (&lv_font_montserrat_28)
#elif LV_FONT_MONTSERRAT_24
#define WEATHER_3DAY_TEMP_FONT (&lv_font_montserrat_24)
#else
#define WEATHER_3DAY_TEMP_FONT WEATHER_TEMP_FONT
#endif

#if LV_FONT_MONTSERRAT_18
#define WEATHER_3DAY_META_FONT (&lv_font_montserrat_18)
#else
#define WEATHER_3DAY_META_FONT WEATHER_CONDITION_FONT
#endif

#define WEATHER_3DAY_ROWS 4
#define WEATHER_3DAY_TRACK_BG 0x4A5D6D
#define WEATHER_3DAY_FILL_COLD 0x79CDE0
#define WEATHER_3DAY_FILL_WARM 0xF4B764
#define WEATHER_3DAY_MARKER_RING 0x2E3C49

typedef struct {
    bool valid;
    char day[12];
    bool has_high;
    bool has_low;
    float high_temp;
    float low_temp;
    char condition_key[32];
    char condition[24];
} weather_forecast_t;

typedef struct {
    bool has_temp;
    float temp;
    int humidity;
    char unit[12];
    char condition_key[32];
    char condition[32];
    bool today_has_high;
    bool today_has_low;
    float today_high_temp;
    float today_low_temp;
    char today_condition_key[32];
    weather_forecast_t forecast[3];
} weather_values_t;

typedef struct {
    lv_obj_t *container;
    lv_obj_t *day_label;
    lv_obj_t *icon_label;
    lv_obj_t *low_label;
    lv_obj_t *bar_track;
    lv_obj_t *bar_fill;
    lv_obj_t *bar_marker;
    lv_obj_t *high_label;
} weather_3day_row_widgets_t;

typedef struct {
    bool valid;
    bool has_low;
    bool has_high;
    bool has_point;
    float low_temp;
    float high_temp;
    float point_temp;
    char day[12];
    char condition_key[32];
} weather_3day_row_t;

typedef struct {
    bool show_forecast;
    lv_obj_t *condition_label;
    lv_obj_t *temp_label;
    lv_obj_t *meta_label;
    weather_3day_row_widgets_t rows[4];
    lv_obj_t *lottie_icon;
    void *lottie_buf;
    size_t lottie_buf_size;
    lv_coord_t lottie_size;
    const void *last_lottie_src;
    size_t last_lottie_src_size;
    lv_coord_t configured_min_dim;
    uint32_t last_icon_cp;
    const lv_font_t *last_icon_font;
    char last_condition_text[32];
} w_weather_tile_ctx_t;

#ifndef APP_UI_WEATHER_ICON_ALLOW_72
#define APP_UI_WEATHER_ICON_ALLOW_72 1
#endif

#if APP_UI_WEATHER_ICON_DEBUG
static const char *TAG = "w_weather_tile";
#endif

#if APP_UI_WEATHER_LOTTIE_ENABLED
typedef struct {
    const uint8_t *start;
    const uint8_t *end;
} weather_lottie_src_t;

extern const uint8_t weather_lottie_clear_day_start[] asm("_binary_weather_clear_day_json_start");
extern const uint8_t weather_lottie_clear_day_end[] asm("_binary_weather_clear_day_json_end");
extern const uint8_t weather_lottie_clear_night_start[] asm("_binary_weather_clear_night_json_start");
extern const uint8_t weather_lottie_clear_night_end[] asm("_binary_weather_clear_night_json_end");
extern const uint8_t weather_lottie_cloudy_start[] asm("_binary_weather_cloudy_json_start");
extern const uint8_t weather_lottie_cloudy_end[] asm("_binary_weather_cloudy_json_end");
extern const uint8_t weather_lottie_fog_start[] asm("_binary_weather_fog_json_start");
extern const uint8_t weather_lottie_fog_end[] asm("_binary_weather_fog_json_end");
extern const uint8_t weather_lottie_hail_start[] asm("_binary_weather_hail_json_start");
extern const uint8_t weather_lottie_hail_end[] asm("_binary_weather_hail_json_end");
extern const uint8_t weather_lottie_partly_cloudy_day_start[] asm("_binary_weather_partly_cloudy_day_json_start");
extern const uint8_t weather_lottie_partly_cloudy_day_end[] asm("_binary_weather_partly_cloudy_day_json_end");
extern const uint8_t weather_lottie_partly_cloudy_night_start[] asm("_binary_weather_partly_cloudy_night_json_start");
extern const uint8_t weather_lottie_partly_cloudy_night_end[] asm("_binary_weather_partly_cloudy_night_json_end");
extern const uint8_t weather_lottie_sleet_start[] asm("_binary_weather_sleet_json_start");
extern const uint8_t weather_lottie_sleet_end[] asm("_binary_weather_sleet_json_end");
extern const uint8_t weather_lottie_wind_start[] asm("_binary_weather_wind_json_start");
extern const uint8_t weather_lottie_wind_end[] asm("_binary_weather_wind_json_end");
extern const uint8_t weather_lottie_overcast_day_start[] asm("_binary_weather_overcast_day_json_start");
extern const uint8_t weather_lottie_overcast_day_end[] asm("_binary_weather_overcast_day_json_end");
extern const uint8_t weather_lottie_extreme_start[] asm("_binary_weather_extreme_json_start");
extern const uint8_t weather_lottie_extreme_end[] asm("_binary_weather_extreme_json_end");
extern const uint8_t weather_lottie_extreme_rain_start[] asm("_binary_weather_extreme_rain_json_start");
extern const uint8_t weather_lottie_extreme_rain_end[] asm("_binary_weather_extreme_rain_json_end");
extern const uint8_t weather_lottie_rain_start[] asm("_binary_weather_rain_json_start");
extern const uint8_t weather_lottie_rain_end[] asm("_binary_weather_rain_json_end");
extern const uint8_t weather_lottie_snow_start[] asm("_binary_weather_snow_json_start");
extern const uint8_t weather_lottie_snow_end[] asm("_binary_weather_snow_json_end");
extern const uint8_t weather_lottie_thunderstorms_start[] asm("_binary_weather_thunderstorms_json_start");
extern const uint8_t weather_lottie_thunderstorms_end[] asm("_binary_weather_thunderstorms_json_end");
extern const uint8_t weather_lottie_thunderstorms_rain_start[] asm("_binary_weather_thunderstorms_rain_json_start");
extern const uint8_t weather_lottie_thunderstorms_rain_end[] asm("_binary_weather_thunderstorms_rain_json_end");
#endif

static lv_coord_t weather_condition_text_width(lv_obj_t *card)
{
    if (card == NULL) {
        return 0;
    }
    lv_coord_t width = lv_obj_get_width(card) - 32;
    return (width > 0) ? width : 0;
}

static lv_coord_t weather_card_min_dim(lv_obj_t *card)
{
    if (card == NULL) {
        return 0;
    }
    lv_obj_update_layout(card);
    lv_coord_t w = lv_obj_get_width(card);
    lv_coord_t h = lv_obj_get_height(card);

    if (w <= 0) {
        w = lv_obj_get_style_width(card, LV_PART_MAIN);
    }
    if (h <= 0) {
        h = lv_obj_get_style_height(card, LV_PART_MAIN);
    }

    if (w <= 0 || h <= 0) {
        return 0;
    }
    return (w < h) ? w : h;
}

static lv_coord_t weather_effective_min_dim(lv_obj_t *card, const w_weather_tile_ctx_t *ctx)
{
    lv_coord_t min_dim = weather_card_min_dim(card);
    if (min_dim > 0) {
        return min_dim;
    }
    if (ctx != NULL && ctx->configured_min_dim > 0) {
        return ctx->configured_min_dim;
    }
    return 0;
}

static const lv_font_t *weather_pick_temp_font(lv_obj_t *card)
{
    lv_coord_t min_dim = weather_card_min_dim(card);
    if (min_dim >= 300) {
        return WEATHER_TEMP_FONT_LARGE;
    }
    return WEATHER_TEMP_FONT;
}

static const lv_font_t *weather_pick_meta_font(lv_obj_t *card)
{
    lv_coord_t min_dim = weather_card_min_dim(card);
    if (min_dim >= 300) {
        return WEATHER_META_FONT_LARGE;
    }
    return WEATHER_CONDITION_FONT;
}

static void weather_copy_text(char *dst, size_t dst_size, const char *src)
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

static void weather_normalize_condition_key(const char *src, char *dst, size_t dst_size)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }

    if (src == NULL || src[0] == '\0') {
        dst[0] = '\0';
        return;
    }

    size_t out = 0;
    for (size_t i = 0; src[i] != '\0' && out + 1 < dst_size; i++) {
        char c = src[i];
        if (c == ' ' || c == '_') {
            c = '-';
        }
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c + ('a' - 'A'));
        }
        dst[out++] = c;
    }
    dst[out] = '\0';
}

static bool weather_icon_utf8_from_codepoint(uint32_t codepoint, char out[5])
{
    if (out == NULL) {
        return false;
    }
    out[0] = '\0';

    if (codepoint <= 0x7FU) {
        out[0] = (char)codepoint;
        out[1] = '\0';
        return true;
    }
    if (codepoint <= 0x7FFU) {
        out[0] = (char)(0xC0U | ((codepoint >> 6) & 0x1FU));
        out[1] = (char)(0x80U | (codepoint & 0x3FU));
        out[2] = '\0';
        return true;
    }
    if (codepoint <= 0xFFFFU) {
        out[0] = (char)(0xE0U | ((codepoint >> 12) & 0x0FU));
        out[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        out[2] = (char)(0x80U | (codepoint & 0x3FU));
        out[3] = '\0';
        return true;
    }

    out[0] = (char)(0xF0U | ((codepoint >> 18) & 0x07U));
    out[1] = (char)(0x80U | ((codepoint >> 12) & 0x3FU));
    out[2] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
    out[3] = (char)(0x80U | (codepoint & 0x3FU));
    out[4] = '\0';
    return true;
}

static bool weather_parse_float_token(const char *text, float *out)
{
    if (text == NULL || out == NULL) {
        return false;
    }
    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }
    if (*text == '\0') {
        return false;
    }
    char *end = NULL;
    float value = strtof(text, &end);
    if (end == text) {
        return false;
    }
    *out = value;
    return true;
}

static bool weather_parse_int_token(const char *text, int *out)
{
    if (text == NULL || out == NULL) {
        return false;
    }
    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }
    if (*text == '\0') {
        return false;
    }
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text) {
        return false;
    }
    *out = (int)value;
    return true;
}

static bool weather_json_item_to_float(cJSON *item, float *out)
{
    if (item == NULL || out == NULL) {
        return false;
    }
    if (cJSON_IsNumber(item)) {
        *out = (float)item->valuedouble;
        return true;
    }
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        return weather_parse_float_token(item->valuestring, out);
    }
    return false;
}

static bool weather_json_item_to_int(cJSON *item, int *out)
{
    if (item == NULL || out == NULL) {
        return false;
    }
    if (cJSON_IsNumber(item)) {
        *out = item->valueint;
        return true;
    }
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        return weather_parse_int_token(item->valuestring, out);
    }
    return false;
}

static const char *weather_find_json_key(const char *json, const char *key)
{
    if (json == NULL || key == NULL || key[0] == '\0') {
        return NULL;
    }

    char needle[64] = {0};
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    return strstr(json, needle);
}

static bool weather_extract_raw_number_attr(const char *json, const char *key, float *out)
{
    const char *pos = weather_find_json_key(json, key);
    if (pos == NULL) {
        return false;
    }

    pos = strchr(pos, ':');
    if (pos == NULL) {
        return false;
    }
    pos++;
    while (*pos != '\0' && isspace((unsigned char)*pos)) {
        pos++;
    }

    if (*pos == '"') {
        pos++;
        return weather_parse_float_token(pos, out);
    }
    return weather_parse_float_token(pos, out);
}

static bool weather_extract_raw_int_attr(const char *json, const char *key, int *out)
{
    const char *pos = weather_find_json_key(json, key);
    if (pos == NULL) {
        return false;
    }

    pos = strchr(pos, ':');
    if (pos == NULL) {
        return false;
    }
    pos++;
    while (*pos != '\0' && isspace((unsigned char)*pos)) {
        pos++;
    }

    if (*pos == '"') {
        pos++;
        return weather_parse_int_token(pos, out);
    }
    return weather_parse_int_token(pos, out);
}

static bool weather_extract_raw_string_attr(const char *json, const char *key, char *out, size_t out_size)
{
    const char *pos = weather_find_json_key(json, key);
    if (pos == NULL || out == NULL || out_size == 0) {
        return false;
    }

    pos = strchr(pos, ':');
    if (pos == NULL) {
        return false;
    }
    pos++;
    while (*pos != '\0' && isspace((unsigned char)*pos)) {
        pos++;
    }

    if (*pos != '"') {
        return false;
    }
    pos++;

    size_t written = 0;
    while (*pos != '\0' && *pos != '"' && written + 1 < out_size) {
        if (*pos == '\\' && pos[1] != '\0') {
            pos++;
        }
        out[written++] = *pos++;
    }
    out[written] = '\0';
    return written > 0;
}

static bool weather_has_token(const char *key, const char *token)
{
    return (key != NULL && token != NULL && strstr(key, token) != NULL);
}

static bool weather_has_alpha(const char *text)
{
    if (text == NULL) {
        return false;
    }
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (isalpha((unsigned char)text[i])) {
            return true;
        }
    }
    return false;
}

#if APP_UI_WEATHER_LOTTIE_ENABLED
static weather_lottie_src_t weather_pick_lottie_src(const char *key)
{
    weather_lottie_src_t none = {0};
    if (key == NULL || key[0] == '\0') {
        return none;
    }

    /* Home Assistant weather conditions (exact mapping). */
    if (strcmp(key, "sunny") == 0) {
        return (weather_lottie_src_t){weather_lottie_clear_day_start, weather_lottie_clear_day_end};
    }
    if (strcmp(key, "clear-night") == 0) {
        return (weather_lottie_src_t){weather_lottie_clear_night_start, weather_lottie_clear_night_end};
    }
    if (strcmp(key, "partlycloudy") == 0) {
        return (weather_lottie_src_t){weather_lottie_partly_cloudy_day_start, weather_lottie_partly_cloudy_day_end};
    }
    if (strcmp(key, "cloudy") == 0) {
        return (weather_lottie_src_t){weather_lottie_cloudy_start, weather_lottie_cloudy_end};
    }
    if (strcmp(key, "fog") == 0) {
        return (weather_lottie_src_t){weather_lottie_fog_start, weather_lottie_fog_end};
    }
    if (strcmp(key, "hail") == 0) {
        return (weather_lottie_src_t){weather_lottie_hail_start, weather_lottie_hail_end};
    }
    if (strcmp(key, "lightning") == 0) {
        return (weather_lottie_src_t){weather_lottie_thunderstorms_start, weather_lottie_thunderstorms_end};
    }
    if (strcmp(key, "lightning-rainy") == 0) {
        return (weather_lottie_src_t){weather_lottie_thunderstorms_rain_start, weather_lottie_thunderstorms_rain_end};
    }
    if (strcmp(key, "rainy") == 0) {
        return (weather_lottie_src_t){weather_lottie_rain_start, weather_lottie_rain_end};
    }
    if (strcmp(key, "pouring") == 0) {
        return (weather_lottie_src_t){weather_lottie_extreme_rain_start, weather_lottie_extreme_rain_end};
    }
    if (strcmp(key, "snowy") == 0) {
        return (weather_lottie_src_t){weather_lottie_snow_start, weather_lottie_snow_end};
    }
    if (strcmp(key, "snowy-rainy") == 0) {
        return (weather_lottie_src_t){weather_lottie_sleet_start, weather_lottie_sleet_end};
    }
    if (strcmp(key, "windy") == 0 || strcmp(key, "windy-variant") == 0) {
        return (weather_lottie_src_t){weather_lottie_wind_start, weather_lottie_wind_end};
    }
    if (strcmp(key, "exceptional") == 0) {
        return (weather_lottie_src_t){weather_lottie_extreme_start, weather_lottie_extreme_end};
    }

    /* Common aliases from integrations/providers outside the HA core condition set. */
    if (strcmp(key, "clear") == 0) {
        return (weather_lottie_src_t){weather_lottie_clear_day_start, weather_lottie_clear_day_end};
    }
    if (strcmp(key, "partly-cloudy") == 0) {
        return (weather_lottie_src_t){weather_lottie_partly_cloudy_day_start, weather_lottie_partly_cloudy_day_end};
    }

    if (weather_has_token(key, "lightning") || weather_has_token(key, "thunder")) {
        return (weather_lottie_src_t){weather_lottie_thunderstorms_start, weather_lottie_thunderstorms_end};
    }
    if (weather_has_token(key, "snow") || weather_has_token(key, "sleet")) {
        return (weather_lottie_src_t){weather_lottie_snow_start, weather_lottie_snow_end};
    }
    if (weather_has_token(key, "rain") || weather_has_token(key, "pouring") || weather_has_token(key, "drizzle")) {
        return (weather_lottie_src_t){weather_lottie_rain_start, weather_lottie_rain_end};
    }
    if (weather_has_token(key, "night") && weather_has_token(key, "partly")) {
        return (weather_lottie_src_t){weather_lottie_partly_cloudy_night_start, weather_lottie_partly_cloudy_night_end};
    }
    if (weather_has_token(key, "partly")) {
        return (weather_lottie_src_t){weather_lottie_partly_cloudy_day_start, weather_lottie_partly_cloudy_day_end};
    }
    if (weather_has_token(key, "night")) {
        return (weather_lottie_src_t){weather_lottie_clear_night_start, weather_lottie_clear_night_end};
    }
    if (weather_has_token(key, "cloud") || weather_has_token(key, "overcast") || weather_has_token(key, "fog") ||
        weather_has_token(key, "mist") || weather_has_token(key, "haze") || weather_has_token(key, "smoke") ||
        weather_has_token(key, "wind")) {
        return (weather_lottie_src_t){weather_lottie_overcast_day_start, weather_lottie_overcast_day_end};
    }
    if (weather_has_token(key, "sunny") || weather_has_token(key, "clear")) {
        return (weather_lottie_src_t){weather_lottie_clear_day_start, weather_lottie_clear_day_end};
    }
    return none;
}

static bool weather_has_lottie_for_values(const weather_values_t *values)
{
    if (values == NULL) {
        return false;
    }
    weather_lottie_src_t src = weather_pick_lottie_src(values->condition_key);
    return (src.start != NULL && src.end != NULL && src.end > src.start);
}

static lv_coord_t weather_pick_lottie_size(lv_obj_t *card, const w_weather_tile_ctx_t *ctx)
{
    lv_coord_t min_dim = weather_effective_min_dim(card, ctx);
    if (ctx != NULL && ctx->show_forecast) {
        if (min_dim >= 320) {
            return 132;
        }
        if (min_dim >= 280) {
            return 122;
        }
        if (min_dim >= 240) {
            return 110;
        }
        return 96;
    }

    if (min_dim >= 320) {
        return 132;
    }
    if (min_dim >= 280) {
        return 118;
    }
    if (min_dim >= 240) {
        return 104;
    }
    return 88;
}

static bool weather_prepare_lottie_buffer(w_weather_tile_ctx_t *ctx, lv_coord_t size)
{
    if (ctx == NULL || ctx->lottie_icon == NULL || size <= 0) {
        return false;
    }

    size_t pixel_count = (size_t)size * (size_t)size;
    if (pixel_count == 0U || pixel_count > (SIZE_MAX / 4U)) {
        return false;
    }
    size_t bytes = pixel_count * 4U;
    size_t alloc_size = bytes + (size_t)LV_DRAW_BUF_ALIGN;

    if (ctx->lottie_buf != NULL && ctx->lottie_buf_size >= alloc_size && ctx->lottie_size == size) {
        return true;
    }

    if (ctx->lottie_buf != NULL) {
        lv_free(ctx->lottie_buf);
        ctx->lottie_buf = NULL;
        ctx->lottie_buf_size = 0U;
        ctx->lottie_size = 0;
        ctx->last_lottie_src = NULL;
        ctx->last_lottie_src_size = 0U;
    }

    ctx->lottie_buf = lv_malloc(alloc_size);
    if (ctx->lottie_buf == NULL) {
        return false;
    }

    memset(ctx->lottie_buf, 0, alloc_size);
    ctx->lottie_buf_size = alloc_size;
    ctx->lottie_size = size;
    lv_lottie_set_buffer(ctx->lottie_icon, size, size, ctx->lottie_buf);
    return true;
}

static void weather_hide_lottie(w_weather_tile_ctx_t *ctx)
{
    if (ctx == NULL || ctx->lottie_icon == NULL) {
        return;
    }
    lv_obj_add_flag(ctx->lottie_icon, LV_OBJ_FLAG_HIDDEN);
}

static bool weather_show_lottie(lv_obj_t *card, w_weather_tile_ctx_t *ctx, const weather_values_t *values, lv_coord_t icon_x,
    lv_coord_t icon_y, lv_coord_t requested_size)
{
    if (card == NULL || ctx == NULL || values == NULL || ctx->lottie_icon == NULL) {
        weather_hide_lottie(ctx);
        return false;
    }

    weather_lottie_src_t src = weather_pick_lottie_src(values->condition_key);
    if (src.start == NULL || src.end == NULL || src.end <= src.start) {
        weather_hide_lottie(ctx);
        return false;
    }

    lv_coord_t lottie_size = (requested_size > 0) ? requested_size : weather_pick_lottie_size(card, ctx);
    if (!weather_prepare_lottie_buffer(ctx, lottie_size)) {
        weather_hide_lottie(ctx);
        return false;
    }

    size_t src_size = (size_t)(src.end - src.start);
    if (ctx->last_lottie_src != src.start || ctx->last_lottie_src_size != src_size) {
        lv_lottie_set_src_data(ctx->lottie_icon, src.start, src_size);
        ctx->last_lottie_src = src.start;
        ctx->last_lottie_src_size = src_size;
    }

    lv_obj_set_pos(ctx->lottie_icon, icon_x, icon_y);
    lv_obj_clear_flag(ctx->lottie_icon, LV_OBJ_FLAG_HIDDEN);
    return true;
}

static void weather_free_lottie(w_weather_tile_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->lottie_buf != NULL) {
        lv_free(ctx->lottie_buf);
        ctx->lottie_buf = NULL;
        ctx->lottie_buf_size = 0U;
    }
    ctx->lottie_size = 0;
    ctx->last_lottie_src = NULL;
    ctx->last_lottie_src_size = 0U;
}
#else
static bool weather_has_lottie_for_values(const weather_values_t *values)
{
    LV_UNUSED(values);
    return false;
}

static void weather_hide_lottie(w_weather_tile_ctx_t *ctx)
{
    LV_UNUSED(ctx);
}

static bool weather_show_lottie(lv_obj_t *card, w_weather_tile_ctx_t *ctx, const weather_values_t *values, lv_coord_t icon_x,
    lv_coord_t icon_y, lv_coord_t requested_size)
{
    LV_UNUSED(card);
    LV_UNUSED(ctx);
    LV_UNUSED(values);
    LV_UNUSED(icon_x);
    LV_UNUSED(icon_y);
    LV_UNUSED(requested_size);
    return false;
}

static void weather_free_lottie(w_weather_tile_ctx_t *ctx)
{
    LV_UNUSED(ctx);
}
#endif

static uint32_t weather_icon_codepoint_for_key(const char *key)
{
    /* MDI codepoints (matching your ESPHome glyph list). */
    const uint32_t CP_WEATHER_CLOUDY = 0xF0590U;
    const uint32_t CP_WEATHER_FOG = 0xF0591U;
    const uint32_t CP_WEATHER_HAIL = 0xF0592U;
    const uint32_t CP_WEATHER_LIGHTNING = 0xF0593U;
    const uint32_t CP_WEATHER_NIGHT = 0xF0594U;
    const uint32_t CP_WEATHER_PARTLY_CLOUDY = 0xF0595U;
    const uint32_t CP_WEATHER_POURING = 0xF0596U;
    const uint32_t CP_WEATHER_RAINY = 0xF0597U;
    const uint32_t CP_WEATHER_SNOWY = 0xF0598U;
    const uint32_t CP_WEATHER_SUNNY = 0xF0599U;
    const uint32_t CP_WEATHER_SUNSET = 0xF059AU;
    const uint32_t CP_WEATHER_WINDY = 0xF059DU;
    const uint32_t CP_WEATHER_WINDY_VARIANT = 0xF059EU;
    const uint32_t CP_WEATHER_LIGHTNING_RAINY = 0xF067EU;
    const uint32_t CP_WEATHER_SNOWY_RAINY = 0xF067FU;
    const uint32_t CP_WEATHER_HURRICANE = 0xF0898U;
    const uint32_t CP_WEATHER_NIGHT_PARTLY_CLOUDY = 0xF0F31U;
    const uint32_t CP_WEATHER_PARTLY_LIGHTNING = 0xF0F32U;
    const uint32_t CP_WEATHER_PARTLY_RAINY = 0xF0F33U;
    const uint32_t CP_WEATHER_PARTLY_SNOWY = 0xF0F34U;
    const uint32_t CP_WEATHER_PARTLY_SNOWY_RAINY = 0xF0F35U;
    const uint32_t CP_WEATHER_SNOWY_HEAVY = 0xF0F36U;
    const uint32_t CP_WEATHER_TORNADO = 0xF0F38U;

    if (key == NULL || key[0] == '\0') {
        return 0U;
    }

    if (strcmp(key, "clear-night") == 0) {
        return CP_WEATHER_NIGHT;
    }
    if (strcmp(key, "partlycloudy") == 0 || strcmp(key, "partly-cloudy") == 0) {
        return CP_WEATHER_PARTLY_CLOUDY;
    }

    if (weather_has_token(key, "tornado")) {
        return CP_WEATHER_TORNADO;
    }
    if (weather_has_token(key, "hurricane")) {
        return CP_WEATHER_HURRICANE;
    }
    if (weather_has_token(key, "lightning") && weather_has_token(key, "rain")) {
        return CP_WEATHER_LIGHTNING_RAINY;
    }
    if (weather_has_token(key, "partly") && weather_has_token(key, "lightning")) {
        return CP_WEATHER_PARTLY_LIGHTNING;
    }
    if (weather_has_token(key, "lightning")) {
        return CP_WEATHER_LIGHTNING;
    }
    if (weather_has_token(key, "hail")) {
        return CP_WEATHER_HAIL;
    }
    if (weather_has_token(key, "fog") || weather_has_token(key, "hazy") || weather_has_token(key, "mist")) {
        return CP_WEATHER_FOG;
    }
    if (weather_has_token(key, "partly") && weather_has_token(key, "snow") && weather_has_token(key, "rain")) {
        return CP_WEATHER_PARTLY_SNOWY_RAINY;
    }
    if (weather_has_token(key, "partly") && weather_has_token(key, "snow")) {
        return CP_WEATHER_PARTLY_SNOWY;
    }
    if (weather_has_token(key, "snow") && weather_has_token(key, "rain")) {
        return CP_WEATHER_SNOWY_RAINY;
    }
    if (weather_has_token(key, "snow") && weather_has_token(key, "heavy")) {
        return CP_WEATHER_SNOWY_HEAVY;
    }
    if (weather_has_token(key, "snow")) {
        return CP_WEATHER_SNOWY;
    }
    if (weather_has_token(key, "partly") && weather_has_token(key, "rain")) {
        return CP_WEATHER_PARTLY_RAINY;
    }
    if (weather_has_token(key, "pouring")) {
        return CP_WEATHER_POURING;
    }
    if (weather_has_token(key, "rain")) {
        return CP_WEATHER_RAINY;
    }
    if (weather_has_token(key, "night") && weather_has_token(key, "partly")) {
        return CP_WEATHER_NIGHT_PARTLY_CLOUDY;
    }
    if (weather_has_token(key, "night")) {
        return CP_WEATHER_NIGHT;
    }
    if (weather_has_token(key, "sunset")) {
        return CP_WEATHER_SUNSET;
    }
    if (weather_has_token(key, "sunny") || weather_has_token(key, "clear")) {
        return CP_WEATHER_SUNNY;
    }
    if (weather_has_token(key, "wind") && weather_has_token(key, "variant")) {
        return CP_WEATHER_WINDY_VARIANT;
    }
    if (weather_has_token(key, "wind")) {
        return CP_WEATHER_WINDY;
    }
    if (weather_has_token(key, "partly")) {
        return CP_WEATHER_PARTLY_CLOUDY;
    }
    if (weather_has_token(key, "cloud")) {
        return CP_WEATHER_CLOUDY;
    }
    return 0U;
}

static const lv_font_t *weather_find_icon_font_for_cp(uint32_t codepoint)
{
    (void)codepoint;
    const lv_font_t *font = mdi_font_weather();
    if (font != NULL) {
        return font;
    }
    return mdi_font_large();
}

static bool weather_font_has_codepoint(const lv_font_t *font, uint32_t codepoint)
{
    if (font == NULL || codepoint == 0U) {
        return false;
    }
    lv_font_glyph_dsc_t dsc = {0};
    return lv_font_get_glyph_dsc(font, &dsc, codepoint, 0);
}

static bool weather_font_has_render_headroom(const lv_font_t *font, uint32_t codepoint)
{
#if LV_USE_STDLIB_MALLOC != LV_STDLIB_BUILTIN
    (void)font;
    (void)codepoint;
    /* With CLIB/custom allocators we don't have meaningful LVGL pool headroom
     * telemetry here; rely on runtime allocator and glyph probing only. */
    return true;
#else
    if (font == NULL || codepoint == 0U) {
        return false;
    }

    lv_font_glyph_dsc_t dsc = {0};
    if (!lv_font_get_glyph_dsc(font, &dsc, codepoint, 0)) {
        return false;
    }

    /* Static A8 glyphs don't need a transient draw buffer allocation. */
    if (lv_font_has_static_bitmap(font) && dsc.format == LV_FONT_GLYPH_FORMAT_A8) {
        return true;
    }

    if (dsc.box_w == 0 || dsc.box_h == 0) {
        return false;
    }

    uint32_t rounded_h = LV_ROUND_UP((uint32_t)dsc.box_h, 32U);
    uint32_t stride = lv_draw_buf_width_to_stride((uint32_t)dsc.box_w, LV_COLOR_FORMAT_A8);
    size_t needed = (size_t)stride * (size_t)rounded_h;

    /* Keep margin for concurrent LVGL allocations in the same frame. */
    const size_t margin = 1024U;
    lv_mem_monitor_t mon = {0};
    lv_mem_monitor(&mon);
    return mon.free_biggest_size > (needed + margin);
#endif
}

#if APP_UI_WEATHER_ICON_DEBUG
static size_t weather_font_render_bytes_required(const lv_font_t *font, uint32_t codepoint)
{
    if (font == NULL || codepoint == 0U) {
        return 0U;
    }

    lv_font_glyph_dsc_t dsc = {0};
    if (!lv_font_get_glyph_dsc(font, &dsc, codepoint, 0)) {
        return 0U;
    }

    if (lv_font_has_static_bitmap(font) && dsc.format == LV_FONT_GLYPH_FORMAT_A8) {
        return 0U;
    }

    if (dsc.box_w == 0 || dsc.box_h == 0) {
        return 0U;
    }

    uint32_t rounded_h = LV_ROUND_UP((uint32_t)dsc.box_h, 32U);
    uint32_t stride = lv_draw_buf_width_to_stride((uint32_t)dsc.box_w, LV_COLOR_FORMAT_A8);
    return (size_t)stride * (size_t)rounded_h;
}
#endif

static void weather_append_unique_font_candidate(
    const lv_font_t **candidates, size_t capacity, size_t *count, const lv_font_t *font)
{
    if (candidates == NULL || count == NULL || capacity == 0 || font == NULL) {
        return;
    }

    for (size_t i = 0; i < *count; i++) {
        if (candidates[i] == font) {
            return;
        }
    }

    if (*count < capacity) {
        candidates[*count] = font;
        (*count)++;
    }
}

static const lv_font_t *weather_pick_render_icon_font(
    lv_obj_t *card, const w_weather_tile_ctx_t *ctx, uint32_t codepoint, const lv_font_t *preferred)
{
    lv_coord_t min_dim = weather_effective_min_dim(card, ctx);
    const lv_font_t *font_56 = mdi_font_large();
#if APP_UI_WEATHER_ICON_ALLOW_72
    const lv_font_t *font_72 = mdi_font_weather();
#else
    const lv_font_t *font_72 = NULL;
#endif
    const lv_font_t *font_100 = mdi_font_weather_100();
    const lv_font_t *font_120 = mdi_font_weather_120();

    /* If size is unresolved during early render, stay conservative and avoid
     * selecting a very large icon font too early. */
    if (min_dim <= 0) {
        min_dim = 240;
    }

    const lv_font_t *candidates[8] = {0};
    size_t count = 0;

    const lv_coord_t tier_72_min_dim = 261;
    const lv_coord_t tier_100_min_dim = 320;
    const lv_coord_t tier_120_min_dim = 420;

    if (min_dim < tier_72_min_dim) {
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_56);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_72);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_100);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_120);
    } else if (min_dim < tier_100_min_dim) {
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_72);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_56);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_100);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_120);
    } else if (min_dim < tier_120_min_dim) {
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_100);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_72);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_120);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_56);
    } else {
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_120);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_100);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_72);
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, font_56);
    }

    if (preferred != NULL) {
        weather_append_unique_font_candidate(candidates, sizeof(candidates) / sizeof(candidates[0]), &count, preferred);
    } else {
        weather_append_unique_font_candidate(
            candidates, sizeof(candidates) / sizeof(candidates[0]), &count, weather_find_icon_font_for_cp(codepoint));
    }

#if APP_UI_WEATHER_ICON_DEBUG
    lv_coord_t card_w = (card != NULL) ? lv_obj_get_width(card) : 0;
    lv_coord_t card_h = (card != NULL) ? lv_obj_get_height(card) : 0;
    bool use_builtin_malloc = (LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN);
    lv_mem_monitor_t mon = {0};
    if (use_builtin_malloc) {
        lv_mem_monitor(&mon);
    }
    size_t need56 = weather_font_render_bytes_required(font_56, codepoint);
    size_t need72 = weather_font_render_bytes_required(font_72, codepoint);
    size_t need100 = weather_font_render_bytes_required(font_100, codepoint);
    size_t need120 = weather_font_render_bytes_required(font_120, codepoint);
    ESP_LOGI("w_weather_tile",
        "card=%dx%d min=%d cp=0x%lX has56=%d has72=%d has100=%d has120=%d fit56=%d fit72=%d fit100=%d fit120=%d need56=%u need72=%u need100=%u need120=%u free_big=%u lv_malloc_builtin=%d LV_FONT_FMT_TXT_LARGE=%d",
        (int)card_w, (int)card_h, (int)min_dim,
        (unsigned long)codepoint,
        weather_font_has_codepoint(font_56, codepoint),
        weather_font_has_codepoint(font_72, codepoint),
        weather_font_has_codepoint(font_100, codepoint),
        weather_font_has_codepoint(font_120, codepoint),
        weather_font_has_render_headroom(font_56, codepoint),
        weather_font_has_render_headroom(font_72, codepoint),
        weather_font_has_render_headroom(font_100, codepoint),
        weather_font_has_render_headroom(font_120, codepoint),
        (unsigned int)need56,
        (unsigned int)need72,
        (unsigned int)need100,
        (unsigned int)need120,
        use_builtin_malloc ? (unsigned int)mon.free_biggest_size : 0U,
        use_builtin_malloc ? 1 : 0,
        (int)LV_FONT_FMT_TXT_LARGE);
#endif

    for (size_t i = 0; i < count; i++) {
        if (weather_font_has_codepoint(candidates[i], codepoint) &&
            weather_font_has_render_headroom(candidates[i], codepoint)) {
            return candidates[i];
        }
    }

    /* Fallback if glyph probing fails unexpectedly on this platform/build. */
    for (size_t i = 0; i < count; i++) {
        if (candidates[i] != NULL) {
            return candidates[i];
        }
    }

    if (preferred != NULL) {
        return preferred;
    }
    return weather_find_icon_font_for_cp(codepoint);
}

static void weather_humanize_condition(const char *src, char *dst, size_t dst_size)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }
    if (src == NULL || src[0] == '\0') {
        weather_copy_text(dst, dst_size, "--");
        return;
    }

    size_t out = 0;
    bool uppercase_next = true;
    for (size_t i = 0; src[i] != '\0' && out + 1 < dst_size; i++) {
        char c = src[i];
        if (c == '_' || c == '-') {
            c = ' ';
        }
        if (uppercase_next && c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        dst[out++] = c;
        uppercase_next = (c == ' ');
    }
    dst[out] = '\0';
}

static void weather_update_icon_cache_from_state(w_weather_tile_ctx_t *ctx, const char *state_text)
{
    if (ctx == NULL || state_text == NULL || state_text[0] == '\0') {
        return;
    }

    char key[32] = {0};
    char human[32] = {0};
    weather_normalize_condition_key(state_text, key, sizeof(key));
    uint32_t cp = weather_icon_codepoint_for_key(key);
    if (cp == 0U) {
        return;
    }

    weather_humanize_condition(state_text, human, sizeof(human));
    ctx->last_icon_cp = cp;
    weather_copy_text(ctx->last_condition_text, sizeof(ctx->last_condition_text), human);
}

static bool weather_parse_ymd(const char *datetime, int *year, int *month, int *day)
{
    if (datetime == NULL || strlen(datetime) < 10 || datetime[4] != '-' || datetime[7] != '-') {
        return false;
    }

    for (size_t i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (!isdigit((unsigned char)datetime[i])) {
            return false;
        }
    }

    int y = (datetime[0] - '0') * 1000 + (datetime[1] - '0') * 100 + (datetime[2] - '0') * 10 + (datetime[3] - '0');
    int m = (datetime[5] - '0') * 10 + (datetime[6] - '0');
    int d = (datetime[8] - '0') * 10 + (datetime[9] - '0');
    if (m < 1 || m > 12 || d < 1 || d > 31) {
        return false;
    }

    if (year != NULL) {
        *year = y;
    }
    if (month != NULL) {
        *month = m;
    }
    if (day != NULL) {
        *day = d;
    }
    return true;
}

static int weather_weekday_from_ymd(int year, int month, int day)
{
    static const int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) {
        year -= 1;
    }
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

static void weather_day_from_datetime(const char *datetime, char *out, size_t out_size)
{
    static const char *weekday_names[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    if (out == NULL || out_size == 0) {
        return;
    }

    int year = 0;
    int month = 0;
    int day = 0;
    if (!weather_parse_ymd(datetime, &year, &month, &day)) {
        weather_copy_text(out, out_size, "--");
        return;
    }

    int weekday = weather_weekday_from_ymd(year, month, day);
    if (weekday < 0 || weekday > 6) {
        weather_copy_text(out, out_size, "--");
        return;
    }
    weather_copy_text(out, out_size, weekday_names[weekday]);
}

static bool weather_datetime_is_today(const char *datetime)
{
    int year = 0;
    int month = 0;
    int day = 0;
    if (!weather_parse_ymd(datetime, &year, &month, &day)) {
        return false;
    }

    time_t now = time(NULL);
    struct tm local_now = {0};
    localtime_r(&now, &local_now);

    return (year == (local_now.tm_year + 1900)) && (month == (local_now.tm_mon + 1)) && (day == local_now.tm_mday);
}

static void weather_values_default(weather_values_t *values)
{
    if (values == NULL) {
        return;
    }
    memset(values, 0, sizeof(*values));
    values->humidity = -1;
    values->condition_key[0] = '\0';
    values->today_condition_key[0] = '\0';
    weather_copy_text(values->condition, sizeof(values->condition), "--");
    weather_copy_text(values->unit, sizeof(values->unit), "C");
    for (size_t i = 0; i < 3; i++) {
        weather_copy_text(values->forecast[i].day, sizeof(values->forecast[i].day), "--");
        values->forecast[i].condition_key[0] = '\0';
        weather_copy_text(values->forecast[i].condition, sizeof(values->forecast[i].condition), "--");
    }
}

static void weather_extract_values(const ha_state_t *state, bool want_forecast, weather_values_t *out)
{
    if (state == NULL || out == NULL) {
        return;
    }
    weather_values_default(out);
    weather_normalize_condition_key(state->state, out->condition_key, sizeof(out->condition_key));
    weather_humanize_condition(state->state, out->condition, sizeof(out->condition));

    const char *attrs_json = state->attributes_json;
    cJSON *attrs = cJSON_Parse(attrs_json);
    if (attrs != NULL) {
        cJSON *temperature = cJSON_GetObjectItemCaseSensitive(attrs, "temperature");
        cJSON *current_temperature = cJSON_GetObjectItemCaseSensitive(attrs, "current_temperature");
        cJSON *native_temperature = cJSON_GetObjectItemCaseSensitive(attrs, "native_temperature");
        if (weather_json_item_to_float(temperature, &out->temp) ||
            weather_json_item_to_float(current_temperature, &out->temp) ||
            weather_json_item_to_float(native_temperature, &out->temp)) {
            out->has_temp = true;
        }

        cJSON *unit = cJSON_GetObjectItemCaseSensitive(attrs, "temperature_unit");
        if (cJSON_IsString(unit) && unit->valuestring != NULL && unit->valuestring[0] != '\0') {
            weather_copy_text(out->unit, sizeof(out->unit), unit->valuestring);
        } else {
            cJSON *native_unit = cJSON_GetObjectItemCaseSensitive(attrs, "native_temperature_unit");
            if (cJSON_IsString(native_unit) && native_unit->valuestring != NULL && native_unit->valuestring[0] != '\0') {
                weather_copy_text(out->unit, sizeof(out->unit), native_unit->valuestring);
            }
        }

        cJSON *humidity = cJSON_GetObjectItemCaseSensitive(attrs, "humidity");
        weather_json_item_to_int(humidity, &out->humidity);
    }

    if (!out->has_temp && attrs_json != NULL && attrs_json[0] != '\0') {
        if (weather_extract_raw_number_attr(attrs_json, "temperature", &out->temp) ||
            weather_extract_raw_number_attr(attrs_json, "current_temperature", &out->temp) ||
            weather_extract_raw_number_attr(attrs_json, "native_temperature", &out->temp)) {
            out->has_temp = true;
        }
    }

    if (out->unit[0] == '\0' || strcmp(out->unit, "C") == 0) {
        char unit_raw[12] = {0};
        if ((attrs_json != NULL && attrs_json[0] != '\0') &&
            (weather_extract_raw_string_attr(attrs_json, "temperature_unit", unit_raw, sizeof(unit_raw)) ||
             weather_extract_raw_string_attr(attrs_json, "native_temperature_unit", unit_raw, sizeof(unit_raw)))) {
            weather_copy_text(out->unit, sizeof(out->unit), unit_raw);
        }
    }

    if (out->humidity < 0 && attrs_json != NULL && attrs_json[0] != '\0') {
        int humidity = -1;
        if (weather_extract_raw_int_attr(attrs_json, "humidity", &humidity)) {
            out->humidity = humidity;
        }
    }

    if (!out->has_temp && state->state[0] != '\0') {
        float state_temp = 0.0f;
        if (weather_parse_float_token(state->state, &state_temp)) {
            out->temp = state_temp;
            out->has_temp = true;
        }
    }

    if (want_forecast) {
        cJSON *forecast = (attrs != NULL) ? cJSON_GetObjectItemCaseSensitive(attrs, "forecast") : NULL;
        if (cJSON_IsArray(forecast)) {
            int count = cJSON_GetArraySize(forecast);
            int out_idx = 0;
            for (int i = 0; i < count && out_idx < 3; i++) {
                cJSON *item = cJSON_GetArrayItem(forecast, i);
                if (!cJSON_IsObject(item)) {
                    continue;
                }

                cJSON *datetime = cJSON_GetObjectItemCaseSensitive(item, "datetime");
                if (!cJSON_IsString(datetime) || datetime->valuestring == NULL) {
                    datetime = cJSON_GetObjectItemCaseSensitive(item, "date");
                }
                bool is_today = cJSON_IsString(datetime) && datetime->valuestring != NULL &&
                                weather_datetime_is_today(datetime->valuestring);

                bool has_high = false;
                float high_temp = 0.0f;
                cJSON *high = cJSON_GetObjectItemCaseSensitive(item, "temperature");
                if (!weather_json_item_to_float(high, &high_temp)) {
                    high = cJSON_GetObjectItemCaseSensitive(item, "native_temperature");
                }
                if (weather_json_item_to_float(high, &high_temp)) {
                    has_high = true;
                }

                bool has_low = false;
                float low_temp = 0.0f;
                cJSON *low = cJSON_GetObjectItemCaseSensitive(item, "templow");
                if (!weather_json_item_to_float(low, &low_temp)) {
                    low = cJSON_GetObjectItemCaseSensitive(item, "native_templow");
                }
                if (weather_json_item_to_float(low, &low_temp)) {
                    has_low = true;
                }

                cJSON *condition = cJSON_GetObjectItemCaseSensitive(item, "condition");
                char condition_key[32] = {0};
                char condition_human[sizeof(out->forecast[0].condition)] = {0};
                if (cJSON_IsString(condition) && condition->valuestring != NULL) {
                    weather_normalize_condition_key(condition->valuestring, condition_key, sizeof(condition_key));
                    weather_humanize_condition(condition->valuestring, condition_human, sizeof(condition_human));
                }

                if (is_today) {
                    if (has_high) {
                        out->today_high_temp = high_temp;
                        out->today_has_high = true;
                    }
                    if (has_low) {
                        out->today_low_temp = low_temp;
                        out->today_has_low = true;
                    }
                    if (condition_key[0] != '\0') {
                        weather_copy_text(out->today_condition_key, sizeof(out->today_condition_key), condition_key);
                    }
                    continue;
                }

                weather_forecast_t *slot = &out->forecast[out_idx];
                slot->valid = true;
                if (cJSON_IsString(datetime) && datetime->valuestring != NULL) {
                    weather_day_from_datetime(datetime->valuestring, slot->day, sizeof(slot->day));
                }
                if (has_high) {
                    slot->has_high = true;
                    slot->high_temp = high_temp;
                }
                if (has_low) {
                    slot->has_low = true;
                    slot->low_temp = low_temp;
                }
                if (condition_key[0] != '\0') {
                    weather_copy_text(slot->condition_key, sizeof(slot->condition_key), condition_key);
                }
                if (condition_human[0] != '\0') {
                    weather_copy_text(slot->condition, sizeof(slot->condition), condition_human);
                }
                out_idx++;
            }
        }
    }

    if (attrs != NULL) {
        cJSON_Delete(attrs);
    }
}

static float weather_clampf(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static float weather_normalize_temp(float value, float range_min, float range_max)
{
    float span = range_max - range_min;
    if (span < 0.001f) {
        return 0.5f;
    }
    float norm = (value - range_min) / span;
    return weather_clampf(norm, 0.0f, 1.0f);
}

static void weather_format_temp(char *dst, size_t dst_size, float temp, const char *unit)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }
    const char *temp_unit = (unit != NULL && unit[0] != '\0') ? unit : "C";
    snprintf(dst, dst_size, "%.0f%s", (double)temp, temp_unit);
}

static void weather_set_row_icon(lv_obj_t *label, const char *condition_key)
{
    if (label == NULL) {
        return;
    }

    uint32_t cp = weather_icon_codepoint_for_key(condition_key);
    const lv_font_t *font = mdi_font_weather_20();
    if (font == NULL) {
        font = mdi_font_weather_small();
    }
    if (!weather_font_has_codepoint(font, cp)) {
        font = weather_find_icon_font_for_cp(cp);
    }

    char utf8[5] = {0};
    if (cp != 0U && font != NULL && weather_icon_utf8_from_codepoint(cp, utf8)) {
        lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
        lv_obj_set_style_text_opa(label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_transform_zoom(label, 256, LV_PART_MAIN);
        lv_label_set_text(label, utf8);
        return;
    }

    lv_obj_set_style_text_font(label, WEATHER_3DAY_META_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(APP_UI_COLOR_TEXT_MUTED), LV_PART_MAIN);
    lv_obj_set_style_transform_zoom(label, 256, LV_PART_MAIN);
    lv_label_set_text(label, "-");
}

static void weather_build_3day_rows(const weather_values_t *values, weather_3day_row_t rows[WEATHER_3DAY_ROWS])
{
    if (values == NULL || rows == NULL) {
        return;
    }

    memset(rows, 0, sizeof(weather_3day_row_t) * WEATHER_3DAY_ROWS);

    weather_3day_row_t *current = &rows[0];
    current->valid = true;
    weather_copy_text(current->day, sizeof(current->day), "Now");
    weather_copy_text(current->condition_key, sizeof(current->condition_key),
        values->today_condition_key[0] != '\0' ? values->today_condition_key : values->condition_key);

    if (values->today_has_low) {
        current->has_low = true;
        current->low_temp = values->today_low_temp;
    }
    if (values->today_has_high) {
        current->has_high = true;
        current->high_temp = values->today_high_temp;
    }

    if (!current->has_low && values->has_temp) {
        current->has_low = true;
        current->low_temp = values->temp;
    }
    if (!current->has_high && values->has_temp) {
        current->has_high = true;
        current->high_temp = values->temp;
    }

    if (current->has_low && !current->has_high) {
        current->has_high = true;
        current->high_temp = current->low_temp;
    } else if (!current->has_low && current->has_high) {
        current->has_low = true;
        current->low_temp = current->high_temp;
    }

    if (values->has_temp) {
        current->has_point = true;
        current->point_temp = values->temp;
    }

    for (int i = 0; i < 3; i++) {
        const weather_forecast_t *src = &values->forecast[i];
        weather_3day_row_t *dst = &rows[i + 1];
        if (!src->valid) {
            continue;
        }

        dst->valid = true;
        dst->has_low = src->has_low;
        dst->has_high = src->has_high;
        dst->low_temp = src->low_temp;
        dst->high_temp = src->high_temp;
        weather_copy_text(dst->day, sizeof(dst->day), src->day);
        weather_copy_text(dst->condition_key, sizeof(dst->condition_key),
            src->condition_key[0] != '\0' ? src->condition_key : values->condition_key);

        if (dst->has_low && !dst->has_high) {
            dst->has_high = true;
            dst->high_temp = dst->low_temp;
        } else if (!dst->has_low && dst->has_high) {
            dst->has_low = true;
            dst->low_temp = dst->high_temp;
        }
    }
}

static void weather_compute_3day_range(
    const weather_3day_row_t rows[WEATHER_3DAY_ROWS], float *out_min, float *out_max)
{
    if (rows == NULL || out_min == NULL || out_max == NULL) {
        return;
    }

    bool has_any = false;
    float min_temp = 0.0f;
    float max_temp = 0.0f;

    for (int i = 0; i < WEATHER_3DAY_ROWS; i++) {
        const weather_3day_row_t *row = &rows[i];
        if (!row->valid) {
            continue;
        }

        if (row->has_low) {
            if (!has_any || row->low_temp < min_temp) {
                min_temp = row->low_temp;
            }
            if (!has_any || row->low_temp > max_temp) {
                max_temp = row->low_temp;
            }
            has_any = true;
        }
        if (row->has_high) {
            if (!has_any || row->high_temp < min_temp) {
                min_temp = row->high_temp;
            }
            if (!has_any || row->high_temp > max_temp) {
                max_temp = row->high_temp;
            }
            has_any = true;
        }
        if (row->has_point) {
            if (!has_any || row->point_temp < min_temp) {
                min_temp = row->point_temp;
            }
            if (!has_any || row->point_temp > max_temp) {
                max_temp = row->point_temp;
            }
            has_any = true;
        }
    }

    if (!has_any) {
        *out_min = 0.0f;
        *out_max = 1.0f;
        return;
    }

    float span = max_temp - min_temp;
    if (span < 1.0f) {
        float mid = (max_temp + min_temp) * 0.5f;
        min_temp = mid - 0.5f;
        max_temp = mid + 0.5f;
        span = 1.0f;
    }

    float pad = span * 0.08f;
    if (pad < 0.5f) {
        pad = 0.5f;
    }

    *out_min = min_temp - pad;
    *out_max = max_temp + pad;
}

static void weather_set_3day_rows_layout(lv_obj_t *card, w_weather_tile_ctx_t *ctx)
{
    if (card == NULL || ctx == NULL) {
        return;
    }

    lv_coord_t card_w = lv_obj_get_width(card);
    lv_coord_t card_h = lv_obj_get_height(card);
    lv_coord_t left = 16;
    lv_coord_t right = 16;
    lv_coord_t content_w = card_w - left - right;
    if (content_w < 120) {
        content_w = 120;
    }

    lv_coord_t rows_top = 150;
    lv_coord_t rows_bottom = card_h - 12;
    lv_coord_t row_gap = 4;
    lv_coord_t available_h = rows_bottom - rows_top;
    lv_coord_t row_h = (available_h - (WEATHER_3DAY_ROWS - 1) * row_gap) / WEATHER_3DAY_ROWS;
    if (row_h < 22) {
        row_h = 22;
        row_gap = 3;
        rows_top = rows_bottom - (WEATHER_3DAY_ROWS * row_h + (WEATHER_3DAY_ROWS - 1) * row_gap);
        if (rows_top < 118) {
            rows_top = 118;
        }
    }

    for (int i = 0; i < WEATHER_3DAY_ROWS; i++) {
        weather_3day_row_widgets_t *row = &ctx->rows[i];
        if (row->container == NULL) {
            continue;
        }

        lv_coord_t y = rows_top + i * (row_h + row_gap);
        lv_obj_set_pos(row->container, left, y);
        lv_obj_set_size(row->container, content_w, row_h);

        lv_coord_t day_w = 50;
        lv_coord_t icon_w = 28;
        lv_coord_t low_w = 46;
        lv_coord_t high_w = 46;
        lv_coord_t gap = 4;
        lv_coord_t bar_w = content_w - (day_w + icon_w + low_w + high_w + gap * 4);
        if (bar_w < 56) {
            day_w = 42;
            icon_w = 24;
            low_w = 42;
            high_w = 42;
            bar_w = content_w - (day_w + icon_w + low_w + high_w + gap * 4);
            if (bar_w < 40) {
                bar_w = 40;
            }
        }

        lv_coord_t x = 0;
        lv_obj_set_pos(row->day_label, x, 0);
        lv_obj_set_size(row->day_label, day_w, row_h);
        x += day_w + gap;

        lv_obj_set_pos(row->icon_label, x, 0);
        lv_obj_set_size(row->icon_label, icon_w, row_h);
        x += icon_w + gap;

        lv_obj_set_pos(row->low_label, x, 0);
        lv_obj_set_size(row->low_label, low_w, row_h);
        x += low_w + gap;

        lv_coord_t track_h = (row_h >= 24) ? 14 : 12;
        const lv_font_t *text_font = lv_obj_get_style_text_font(row->low_label, LV_PART_MAIN);
        if (text_font == NULL) {
            text_font = WEATHER_3DAY_META_FONT;
        }
        lv_coord_t text_h = (text_font != NULL) ? (lv_coord_t)lv_font_get_line_height(text_font) : row_h;
        lv_coord_t track_y = (text_h - track_h) / 2;
        if (track_y < 0) {
            track_y = 0;
        }
        lv_coord_t max_track_y = row_h - track_h;
        if (track_y > max_track_y) {
            track_y = max_track_y;
        }
        lv_obj_set_pos(row->bar_track, x, track_y);
        lv_obj_set_size(row->bar_track, bar_w, track_h);
        x += bar_w + gap;

        lv_obj_set_pos(row->high_label, x, 0);
        lv_obj_set_size(row->high_label, high_w, row_h);
    }
}

static void weather_set_3day_row_values(weather_3day_row_widgets_t *widgets, const weather_3day_row_t *row,
    const char *unit, float range_min, float range_max)
{
    if (widgets == NULL || widgets->container == NULL) {
        return;
    }

    if (row == NULL || !row->valid) {
        lv_label_set_text(widgets->day_label, "--");
        lv_label_set_text(widgets->low_label, "--");
        lv_label_set_text(widgets->high_label, "--");
        weather_set_row_icon(widgets->icon_label, NULL);
        lv_obj_add_flag(widgets->bar_fill, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widgets->bar_marker, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_label_set_text(widgets->day_label, row->day[0] != '\0' ? row->day : "--");
    weather_set_row_icon(widgets->icon_label, row->condition_key);

    char low_text[24] = {0};
    if (row->has_low) {
        weather_format_temp(low_text, sizeof(low_text), row->low_temp, unit);
    } else {
        weather_copy_text(low_text, sizeof(low_text), "--");
    }
    lv_label_set_text(widgets->low_label, low_text);

    char high_text[24] = {0};
    if (row->has_high) {
        weather_format_temp(high_text, sizeof(high_text), row->high_temp, unit);
    } else {
        weather_copy_text(high_text, sizeof(high_text), "--");
    }
    lv_label_set_text(widgets->high_label, high_text);

    lv_obj_update_layout(widgets->bar_track);
    lv_coord_t track_w = lv_obj_get_width(widgets->bar_track);
    lv_coord_t track_h = lv_obj_get_height(widgets->bar_track);
    if (track_w <= 0 || track_h <= 0) {
        lv_obj_add_flag(widgets->bar_fill, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widgets->bar_marker, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (row->has_low && row->has_high) {
        float low_temp = row->low_temp;
        float high_temp = row->high_temp;
        if (low_temp > high_temp) {
            float tmp = low_temp;
            low_temp = high_temp;
            high_temp = tmp;
        }

        float start_norm = weather_normalize_temp(low_temp, range_min, range_max);
        float end_norm = weather_normalize_temp(high_temp, range_min, range_max);
        if (end_norm < start_norm) {
            float tmp = end_norm;
            end_norm = start_norm;
            start_norm = tmp;
        }

        lv_coord_t fill_x = (lv_coord_t)(start_norm * (float)track_w + 0.5f);
        lv_coord_t fill_end = (lv_coord_t)(end_norm * (float)track_w + 0.5f);
        if (fill_end <= fill_x) {
            fill_end = fill_x + 2;
        }
        if (fill_x < 0) {
            fill_x = 0;
        }
        if (fill_end > track_w) {
            fill_end = track_w;
        }
        if (fill_end <= fill_x) {
            fill_x = 0;
            fill_end = track_w;
        }

        lv_obj_set_pos(widgets->bar_fill, fill_x, 0);
        lv_obj_set_size(widgets->bar_fill, fill_end - fill_x, track_h);
        lv_obj_clear_flag(widgets->bar_fill, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(widgets->bar_fill, LV_OBJ_FLAG_HIDDEN);
    }

    if (row->has_point) {
        float point_norm = weather_normalize_temp(row->point_temp, range_min, range_max);
        lv_coord_t marker_size = track_h > 12 ? track_h : 12;
        lv_coord_t marker_x = (lv_coord_t)(point_norm * (float)track_w + 0.5f) - marker_size / 2;
        if (marker_x < 0) {
            marker_x = 0;
        }
        if ((marker_x + marker_size) > track_w) {
            marker_x = track_w - marker_size;
        }
        lv_coord_t marker_y = (track_h - marker_size) / 2;
        lv_obj_set_pos(widgets->bar_marker, marker_x, marker_y);
        lv_obj_set_size(widgets->bar_marker, marker_size, marker_size);
        lv_obj_clear_flag(widgets->bar_marker, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(widgets->bar_marker, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_coord_t weather_content_top_y(void)
{
#if APP_UI_TILE_LAYOUT_TUNED
    return 34;
#else
    return 32;
#endif
}

static lv_coord_t weather_content_bottom_y(lv_obj_t *card, const w_weather_tile_ctx_t *ctx)
{
    if (card == NULL) {
        return 0;
    }

    lv_coord_t card_h = lv_obj_get_height(card);
    lv_coord_t bottom = card_h - 16;

    if (ctx != NULL && ctx->show_forecast) {
        lv_coord_t row_y =
#if APP_UI_TILE_LAYOUT_TUNED
            card_h - 102;
#else
            card_h - 106;
#endif
        if (row_y < 152) {
            row_y = 152;
        }
        bottom = row_y - 8;
    }

    return (bottom > 0) ? bottom : 0;
}

static lv_coord_t weather_label_height_or_font(lv_obj_t *label, const lv_font_t *fallback_font)
{
    if (label == NULL) {
        return (fallback_font != NULL) ? fallback_font->line_height : 16;
    }

    lv_coord_t h = lv_obj_get_height(label);
    if (h > 0) {
        return h;
    }

    const lv_font_t *font = lv_obj_get_style_text_font(label, LV_PART_MAIN);
    if (font != NULL) {
        return font->line_height;
    }
    if (fallback_font != NULL) {
        return fallback_font->line_height;
    }
    return 16;
}

static void weather_compute_main_layout(lv_obj_t *card, w_weather_tile_ctx_t *ctx, bool icon_mode,
    const lv_font_t *icon_font, const lv_font_t *temp_font, const lv_font_t *meta_font,
    lv_coord_t condition_height_override, lv_coord_t *out_condition_y, lv_coord_t *out_temp_y, lv_coord_t *out_meta_y)
{
    if (card == NULL || ctx == NULL || out_condition_y == NULL || out_temp_y == NULL || out_meta_y == NULL) {
        return;
    }

    lv_obj_update_layout(card);

    lv_coord_t top = weather_content_top_y();
    lv_coord_t bottom = weather_content_bottom_y(card, ctx);
    if (bottom <= top) {
        bottom = top + 1;
    }

    lv_coord_t h_condition = condition_height_override > 0
                                 ? condition_height_override
                                 : weather_label_height_or_font(ctx->condition_label, icon_mode ? icon_font : NULL);
    lv_coord_t h_temp = weather_label_height_or_font(ctx->temp_label, temp_font);
    lv_coord_t h_meta = weather_label_height_or_font(ctx->meta_label, meta_font);

    lv_coord_t gap_condition_temp = icon_mode ? 10 : 8;
    lv_coord_t gap_temp_meta = 8;
    lv_coord_t min_dim = weather_card_min_dim(card);
    if (min_dim >= 300) {
        gap_condition_temp = icon_mode ? 14 : 12;
        gap_temp_meta = 12;
    }
    lv_coord_t content_h = h_condition + gap_condition_temp + h_temp + gap_temp_meta + h_meta;
    lv_coord_t avail_h = bottom - top;

    lv_coord_t y0 = top;
    if (avail_h > content_h) {
        y0 = top + (avail_h - content_h) / 2;
        /* Slight downward bias on larger cards to avoid too much empty space below content. */
        if (min_dim >= 300 && ctx->show_forecast == false) {
            lv_coord_t bias = (avail_h - content_h) / 8;
            y0 += bias;
        }
    }

    lv_coord_t condition_y = y0;
    lv_coord_t temp_y = condition_y + h_condition + gap_condition_temp;
    lv_coord_t meta_y = temp_y + h_temp + gap_temp_meta;

    if ((meta_y + h_meta) > bottom) {
        lv_coord_t overflow = (meta_y + h_meta) - bottom;
        if (overflow > 0) {
            meta_y -= overflow;
            if (meta_y < (temp_y + 4)) {
                meta_y = temp_y + 4;
            }
        }
    }

    if (icon_mode && min_dim >= 300 && !ctx->show_forecast) {
        /* Lift icon-centric layout a bit on large cards so the icon is less "low" visually. */
        lv_coord_t lift = 10;
        if (condition_y > (top + 2)) {
            condition_y -= lift;
            temp_y -= (lift / 2);
            meta_y -= (lift / 2);
            if (condition_y < top) {
                condition_y = top;
            }
            if (temp_y < (condition_y + 4)) {
                temp_y = condition_y + 4;
            }
            if (meta_y < (temp_y + 4)) {
                meta_y = temp_y + 4;
            }
        }
    }

    *out_condition_y = condition_y;
    *out_temp_y = temp_y;
    *out_meta_y = meta_y;
}

static lv_coord_t weather_pick_lottie_size_main_adaptive(
    lv_obj_t *card, w_weather_tile_ctx_t *ctx, const lv_font_t *temp_font, const lv_font_t *meta_font)
{
    lv_coord_t fallback = weather_pick_lottie_size(card, ctx);
    if (card == NULL || ctx == NULL) {
        return fallback;
    }

    lv_coord_t top = weather_content_top_y();
    lv_coord_t bottom = weather_content_bottom_y(card, ctx);
    if (bottom <= top) {
        return fallback;
    }

    lv_obj_update_layout(card);
    lv_coord_t min_dim = weather_card_min_dim(card);
    lv_coord_t h_temp = weather_label_height_or_font(ctx->temp_label, temp_font);
    lv_coord_t h_meta = weather_label_height_or_font(ctx->meta_label, meta_font);
    lv_coord_t gap_condition_temp = (min_dim >= 300) ? 14 : 10;
    lv_coord_t gap_temp_meta = (min_dim >= 300) ? 12 : 8;

    lv_coord_t available_h = bottom - top - h_temp - h_meta - gap_condition_temp - gap_temp_meta;
    lv_coord_t available_w = lv_obj_get_width(card) - 36;
    lv_coord_t max_size = (available_h < available_w) ? available_h : available_w;
    if (max_size <= 0) {
        return fallback;
    }

    lv_coord_t size = max_size - 4;
    if (size < 40) {
        size = max_size;
    }
    if (size > (fallback + 28)) {
        size = fallback + 28;
    }
    if (size > max_size) {
        size = max_size;
    }
    if (size < 40) {
        size = 40;
    }
    return size;
}

static void weather_render_3day(lv_obj_t *card, w_weather_tile_ctx_t *ctx, const weather_values_t *values, bool available)
{
    if (card == NULL || ctx == NULL) {
        return;
    }

    lv_coord_t card_w = lv_obj_get_width(card);
    lv_coord_t icon_x = 20;
    lv_coord_t icon_y = 20;

    lv_obj_set_style_text_font(ctx->temp_label, WEATHER_3DAY_TEMP_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_font(ctx->meta_label, WEATHER_3DAY_META_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_align(ctx->temp_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_align(ctx->meta_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(ctx->temp_label, card_w - 32);
    lv_obj_set_width(ctx->meta_label, card_w - 32);
    lv_obj_set_pos(ctx->temp_label, 16, 58);
    lv_obj_set_pos(ctx->meta_label, 16, 102);

    for (int i = 0; i < WEATHER_3DAY_ROWS; i++) {
        weather_3day_row_widgets_t *row = &ctx->rows[i];
        if (row->container == NULL) {
            continue;
        }

        lv_obj_set_style_text_font(row->day_label, WEATHER_3DAY_META_FONT, LV_PART_MAIN);
        lv_obj_set_style_text_color(row->day_label, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
        lv_obj_set_style_text_align(row->day_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_label_set_long_mode(row->day_label, LV_LABEL_LONG_CLIP);

        lv_obj_set_style_text_font(row->icon_label, WEATHER_3DAY_META_FONT, LV_PART_MAIN);
        lv_obj_set_style_text_color(row->icon_label, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
        lv_obj_set_style_text_align(row->icon_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_label_set_long_mode(row->icon_label, LV_LABEL_LONG_CLIP);

        lv_obj_set_style_text_font(row->low_label, WEATHER_3DAY_META_FONT, LV_PART_MAIN);
        lv_obj_set_style_text_color(row->low_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
        lv_obj_set_style_text_align(row->low_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

        lv_obj_set_style_text_font(row->high_label, WEATHER_3DAY_META_FONT, LV_PART_MAIN);
        lv_obj_set_style_text_color(row->high_label, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
        lv_obj_set_style_text_align(row->high_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

        lv_obj_set_style_bg_color(row->bar_track, lv_color_hex(WEATHER_3DAY_TRACK_BG), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(row->bar_track, LV_OPA_60, LV_PART_MAIN);
        lv_obj_set_style_border_width(row->bar_track, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(row->bar_track, LV_RADIUS_CIRCLE, LV_PART_MAIN);

        lv_obj_set_style_bg_color(row->bar_fill, lv_color_hex(WEATHER_3DAY_FILL_COLD), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(row->bar_fill, lv_color_hex(WEATHER_3DAY_FILL_WARM), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(row->bar_fill, LV_GRAD_DIR_HOR, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(row->bar_fill, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(row->bar_fill, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(row->bar_fill, LV_RADIUS_CIRCLE, LV_PART_MAIN);

        lv_obj_set_style_bg_color(row->bar_marker, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(row->bar_marker, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(row->bar_marker, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(row->bar_marker, lv_color_hex(WEATHER_3DAY_MARKER_RING), LV_PART_MAIN);
        lv_obj_set_style_radius(row->bar_marker, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    }
    weather_set_3day_rows_layout(card, ctx);

    if (!available || values == NULL) {
        lv_obj_clear_flag(ctx->condition_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_font(ctx->condition_label, WEATHER_3DAY_TEMP_FONT, LV_PART_MAIN);
        lv_obj_set_style_text_color(ctx->condition_label, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
        lv_obj_set_style_text_align(ctx->condition_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_size(ctx->condition_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_pos(ctx->condition_label, icon_x, icon_y + 8);
        lv_label_set_text(ctx->condition_label, "--");

        lv_label_set_text(ctx->temp_label, "--");
        lv_label_set_text(ctx->meta_label, ui_i18n_get("weather.unavailable", "Unavailable"));
        for (int i = 0; i < WEATHER_3DAY_ROWS; i++) {
            weather_set_3day_row_values(&ctx->rows[i], NULL, "C", 0.0f, 1.0f);
        }
        weather_hide_lottie(ctx);
        return;
    }

    const char *display_condition = (ctx->last_condition_text[0] != '\0') ? ctx->last_condition_text : values->condition;

    uint32_t icon_cp = ctx->last_icon_cp;
    const lv_font_t *icon_font = NULL;
    bool icon_mode = false;
    if (icon_cp != 0U) {
        icon_font = weather_pick_render_icon_font(card, ctx, icon_cp, ctx->last_icon_font);
    }
    if (icon_cp != 0U && icon_font != NULL) {
        char icon_utf8[5] = {0};
        if (weather_icon_utf8_from_codepoint(icon_cp, icon_utf8)) {
            icon_mode = true;
            lv_label_set_long_mode(ctx->condition_label, LV_LABEL_LONG_CLIP);
            lv_obj_set_size(ctx->condition_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_align(ctx->condition_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
            lv_obj_set_style_text_font(ctx->condition_label, icon_font, LV_PART_MAIN);
            lv_obj_set_style_text_color(ctx->condition_label, lv_color_hex(APP_UI_COLOR_WEATHER_ICON), LV_PART_MAIN);
            lv_obj_set_style_text_opa(ctx->condition_label, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_pos(ctx->condition_label, icon_x, icon_y);
            lv_label_set_text(ctx->condition_label, icon_utf8);
            ctx->last_icon_cp = icon_cp;
            ctx->last_icon_font = icon_font;
        }
    }

    if (!icon_mode) {
        lv_label_set_long_mode(ctx->condition_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(ctx->condition_label, card_w - icon_x - 28);
        lv_obj_set_style_text_align(ctx->condition_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_font(ctx->condition_label, WEATHER_3DAY_META_FONT, LV_PART_MAIN);
        lv_obj_set_style_text_color(ctx->condition_label, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
        lv_obj_set_pos(ctx->condition_label, icon_x, icon_y + 16);
        lv_label_set_text(ctx->condition_label, display_condition);
    }

    bool lottie_mode = weather_show_lottie(card, ctx, values, icon_x - 4, icon_y - 8, 0);
    if (lottie_mode) {
        lv_obj_add_flag(ctx->condition_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(ctx->condition_label, LV_OBJ_FLAG_HIDDEN);
    }

    char temp_text[32] = {0};
    if (values->has_temp) {
        snprintf(temp_text, sizeof(temp_text), "%.1f %s", (double)values->temp, values->unit);
    } else {
        snprintf(temp_text, sizeof(temp_text), "--");
    }
    lv_label_set_text(ctx->temp_label, temp_text);

    char meta_text[64] = {0};
    if (values->humidity >= 0) {
        snprintf(meta_text, sizeof(meta_text), "%s | %d%%", display_condition, values->humidity);
    } else {
        snprintf(meta_text, sizeof(meta_text), "%s", display_condition);
    }
    lv_label_set_text(ctx->meta_label, meta_text);

    weather_3day_row_t rows[WEATHER_3DAY_ROWS] = {0};
    weather_build_3day_rows(values, rows);
    float range_min = 0.0f;
    float range_max = 1.0f;
    weather_compute_3day_range(rows, &range_min, &range_max);
    for (int i = 0; i < WEATHER_3DAY_ROWS; i++) {
        weather_set_3day_row_values(&ctx->rows[i], &rows[i], values->unit, range_min, range_max);
    }
}

static void weather_render(lv_obj_t *card, w_weather_tile_ctx_t *ctx, const weather_values_t *values, bool available)
{
    if (card == NULL || ctx == NULL) {
        return;
    }

    lv_obj_set_style_bg_color(card, lv_color_hex(APP_UI_COLOR_CARD_BG_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);

    if (ctx->show_forecast) {
        weather_render_3day(card, ctx, values, available);
        return;
    }

    const lv_font_t *temp_font = weather_pick_temp_font(card);
    const lv_font_t *meta_font = weather_pick_meta_font(card);
    lv_obj_set_style_text_font(ctx->temp_label, temp_font, LV_PART_MAIN);
    lv_obj_set_style_text_font(ctx->meta_label, meta_font, LV_PART_MAIN);

    if (!available || values == NULL) {
        lv_coord_t text_width = weather_condition_text_width(card);
        if (text_width > 0) {
            lv_obj_set_width(ctx->condition_label, text_width);
        }
        lv_obj_set_style_text_font(ctx->condition_label, WEATHER_CONDITION_FONT, LV_PART_MAIN);
        lv_obj_set_style_text_color(ctx->condition_label, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
        lv_obj_clear_flag(ctx->condition_label, LV_OBJ_FLAG_HIDDEN);
#if APP_UI_TILE_LAYOUT_TUNED
        lv_obj_align(ctx->condition_label, LV_ALIGN_TOP_MID, 0, 36);
        lv_obj_align(ctx->temp_label, LV_ALIGN_TOP_MID, 0, 76);
        lv_obj_align(ctx->meta_label, LV_ALIGN_TOP_MID, 0, 130);
#else
        lv_obj_align(ctx->condition_label, LV_ALIGN_TOP_MID, 0, 34);
        lv_obj_align(ctx->temp_label, LV_ALIGN_TOP_MID, 0, 72);
        lv_obj_align(ctx->meta_label, LV_ALIGN_TOP_MID, 0, 124);
#endif
        lv_label_set_text(ctx->condition_label, ui_i18n_get("weather.unavailable", "Unavailable"));
        lv_label_set_text(ctx->temp_label, "--");
        lv_label_set_text(ctx->meta_label, "");
        weather_hide_lottie(ctx);
        return;
    }

    const char *display_condition = values->condition;
    if (ctx->last_condition_text[0] != '\0') {
        display_condition = ctx->last_condition_text;
    }

    uint32_t icon_cp = ctx->last_icon_cp;
    const lv_font_t *icon_font = NULL;
    bool icon_mode = false;
    if (icon_cp != 0U) {
        icon_font = weather_pick_render_icon_font(card, ctx, icon_cp, ctx->last_icon_font);
    }

    if (icon_cp != 0U && icon_font != NULL) {
        icon_mode = true;
        char icon_utf8[5] = {0};
        if (!weather_icon_utf8_from_codepoint(icon_cp, icon_utf8)) {
            icon_mode = false;
        }

        /* Icon rendering: avoid wrapping and width constraints intended for text. */
        lv_label_set_long_mode(ctx->condition_label, LV_LABEL_LONG_CLIP);
        lv_obj_set_size(ctx->condition_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(ctx->condition_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(ctx->condition_label, icon_font, LV_PART_MAIN);
        lv_obj_set_style_text_color(ctx->condition_label, lv_color_hex(APP_UI_COLOR_WEATHER_ICON), LV_PART_MAIN);
        lv_obj_set_style_text_opa(ctx->condition_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_label_set_text(ctx->condition_label, icon_utf8);
        lv_obj_clear_flag(ctx->condition_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(ctx->condition_label);
#if APP_UI_WEATHER_ICON_DEBUG
        lv_coord_t lw = lv_obj_get_width(ctx->condition_label);
        lv_coord_t lh = lv_obj_get_height(ctx->condition_label);
        ESP_LOGI(TAG, "icon font lh=%d label w=%d h=%d text='%s' cp=0x%lX",
            (icon_font != NULL) ? (int)icon_font->line_height : -1, (int)lw, (int)lh, icon_utf8, (unsigned long)icon_cp);
#endif
        lv_obj_move_foreground(ctx->condition_label);
        lv_obj_update_layout(ctx->condition_label);
        lv_obj_update_layout(card);
    }

    if (icon_mode) {
        ctx->last_icon_cp = icon_cp;
        ctx->last_icon_font = icon_font;
        if (ctx->last_condition_text[0] != '\0') {
            display_condition = ctx->last_condition_text;
        }
    }

    if (!icon_mode) {
        lv_coord_t text_width = weather_condition_text_width(card);
        lv_label_set_long_mode(ctx->condition_label, LV_LABEL_LONG_WRAP);
        if (text_width > 0) {
            lv_obj_set_width(ctx->condition_label, text_width);
        }
        lv_obj_set_height(ctx->condition_label, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(ctx->condition_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(ctx->condition_label, WEATHER_CONDITION_FONT, LV_PART_MAIN);
        lv_obj_set_style_text_color(ctx->condition_label, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
        lv_label_set_text(ctx->condition_label, display_condition);
    }

    bool lottie_candidate = weather_has_lottie_for_values(values);
    lv_coord_t lottie_size =
        lottie_candidate ? weather_pick_lottie_size_main_adaptive(card, ctx, temp_font, meta_font) : 0;
    bool visual_icon_mode = icon_mode || lottie_candidate;

    char temp_text[32] = {0};
    if (values->has_temp) {
        snprintf(temp_text, sizeof(temp_text), "%.1f %s", (double)values->temp, values->unit);
    } else {
        snprintf(temp_text, sizeof(temp_text), "--");
    }
    lv_label_set_text(ctx->temp_label, temp_text);

    if (values->humidity >= 0) {
        char meta_text[64] = {0};
        if (visual_icon_mode) {
            snprintf(meta_text, sizeof(meta_text), "%.36s | %d%%", display_condition, values->humidity);
        } else {
            snprintf(meta_text, sizeof(meta_text), ui_i18n_get("weather.humidity_format", "Humidity %d%%"), values->humidity);
        }
        lv_label_set_text(ctx->meta_label, meta_text);
    } else {
        lv_label_set_text(ctx->meta_label, visual_icon_mode ? display_condition : "");
    }

    lv_coord_t condition_y = 0;
    lv_coord_t temp_y = 0;
    lv_coord_t meta_y = 0;
    weather_compute_main_layout(
        card, ctx, visual_icon_mode, icon_font, temp_font, meta_font, lottie_candidate ? lottie_size : 0, &condition_y,
        &temp_y, &meta_y);
    lv_obj_align(ctx->condition_label, LV_ALIGN_TOP_MID, 0, condition_y);
    lv_obj_align(ctx->temp_label, LV_ALIGN_TOP_MID, 0, temp_y);
    lv_obj_align(ctx->meta_label, LV_ALIGN_TOP_MID, 0, meta_y);

    bool lottie_mode = false;
    if (lottie_candidate && lottie_size > 0) {
        lv_coord_t content_w = lv_obj_get_content_width(card);
        if (content_w <= 0) {
            content_w = lv_obj_get_width(card);
        }
        lv_coord_t lottie_x = (content_w - lottie_size) / 2;
        if (lottie_x < 0) {
            lottie_x = 0;
        }
        lv_coord_t min_dim = weather_card_min_dim(card);
        lv_coord_t gap_condition_temp = (min_dim >= 300) ? 14 : 10;
        lv_coord_t slot_top = weather_content_top_y();
        lv_coord_t slot_bottom = temp_y - gap_condition_temp;
        if (slot_bottom <= slot_top) {
            slot_bottom = slot_top + lottie_size;
        }
        lv_coord_t lottie_y = slot_top;
        lv_coord_t slot_h = slot_bottom - slot_top;
        if (slot_h > lottie_size) {
            lottie_y = slot_top + (slot_h - lottie_size) / 2;
        }
        lv_coord_t content_bottom = weather_content_bottom_y(card, ctx);
        if ((lottie_y + lottie_size) > content_bottom) {
            lottie_y = content_bottom - lottie_size;
        }
        if (lottie_y < 0) {
            lottie_y = 0;
        }
        lottie_mode = weather_show_lottie(card, ctx, values, lottie_x, lottie_y, lottie_size);
    } else {
        weather_hide_lottie(ctx);
    }

    if (lottie_mode) {
        lv_obj_add_flag(ctx->condition_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(ctx->condition_label, LV_OBJ_FLAG_HIDDEN);
        if (!lottie_candidate) {
            weather_hide_lottie(ctx);
        }
    }

}

static void w_weather_tile_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_DELETE) {
        return;
    }
    w_weather_tile_ctx_t *ctx = (w_weather_tile_ctx_t *)lv_event_get_user_data(event);
    if (ctx != NULL) {
        weather_free_lottie(ctx);
        free(ctx);
    }
}

esp_err_t w_weather_tile_create(const ui_widget_def_t *def, lv_obj_t *parent, ui_widget_instance_t *out_instance)
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
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
#else
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
#endif

    lv_obj_t *condition = lv_label_create(card);
    lv_label_set_text(condition, "--");
    lv_obj_set_width(condition, def->w - 32);
    lv_obj_set_style_text_align(condition, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(condition, lv_color_hex(APP_UI_COLOR_TEXT_SOFT), LV_PART_MAIN);
    lv_obj_set_style_text_font(condition, WEATHER_CONDITION_FONT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(condition, LV_ALIGN_TOP_MID, 0, 36);
#else
    lv_obj_align(condition, LV_ALIGN_TOP_MID, 0, 34);
#endif

    lv_obj_t *temp = lv_label_create(card);
    lv_label_set_text(temp, "--");
    lv_obj_set_width(temp, def->w - 32);
    lv_obj_set_style_text_align(temp, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(temp, lv_color_hex(APP_UI_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font(temp, WEATHER_TEMP_FONT, LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(temp, LV_ALIGN_TOP_MID, 0, 76);
#else
    lv_obj_align(temp, LV_ALIGN_TOP_MID, 0, 72);
#endif

    lv_obj_t *meta = lv_label_create(card);
    lv_label_set_text(meta, "");
    lv_obj_set_width(meta, def->w - 32);
    lv_obj_set_style_text_align(meta, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(meta, lv_color_hex(APP_UI_COLOR_TEXT_MUTED), LV_PART_MAIN);
#if APP_UI_TILE_LAYOUT_TUNED
    lv_obj_align(meta, LV_ALIGN_TOP_MID, 0, 130);
#else
    lv_obj_align(meta, LV_ALIGN_TOP_MID, 0, 124);
#endif

    w_weather_tile_ctx_t *ctx = calloc(1, sizeof(w_weather_tile_ctx_t));
    if (ctx == NULL) {
        lv_obj_del(card);
        return ESP_ERR_NO_MEM;
    }

    ctx->show_forecast = (strcmp(def->type, "weather_3day") == 0);
    ctx->condition_label = condition;
    ctx->temp_label = temp;
    ctx->meta_label = meta;
    ctx->lottie_icon = NULL;
    ctx->lottie_buf = NULL;
    ctx->lottie_buf_size = 0U;
    ctx->lottie_size = 0;
    ctx->last_lottie_src = NULL;
    ctx->last_lottie_src_size = 0U;
    ctx->configured_min_dim = (def->w < def->h) ? def->w : def->h;
    ctx->last_icon_cp = 0U;
    ctx->last_icon_font = NULL;
    ctx->last_condition_text[0] = '\0';

    if (ctx->show_forecast) {
        for (int i = 0; i < WEATHER_3DAY_ROWS; i++) {
            weather_3day_row_widgets_t *row = &ctx->rows[i];
            row->container = lv_obj_create(card);
            lv_obj_clear_flag(row->container, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_opa(row->container, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(row->container, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(row->container, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(row->container, 0, LV_PART_MAIN);

            row->day_label = lv_label_create(row->container);
            lv_label_set_text(row->day_label, "--");
            lv_obj_set_style_text_align(row->day_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
            lv_label_set_long_mode(row->day_label, LV_LABEL_LONG_CLIP);

            row->icon_label = lv_label_create(row->container);
            lv_label_set_text(row->icon_label, "-");
            lv_obj_set_style_text_align(row->icon_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            lv_label_set_long_mode(row->icon_label, LV_LABEL_LONG_CLIP);

            row->low_label = lv_label_create(row->container);
            lv_label_set_text(row->low_label, "--");
            lv_obj_set_style_text_align(row->low_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

            row->bar_track = lv_obj_create(row->container);
            lv_obj_clear_flag(row->bar_track, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(row->bar_track, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(row->bar_track, 0, LV_PART_MAIN);

            row->bar_fill = lv_obj_create(row->bar_track);
            lv_obj_clear_flag(row->bar_fill, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(row->bar_fill, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(row->bar_fill, 0, LV_PART_MAIN);

            row->bar_marker = lv_obj_create(row->bar_track);
            lv_obj_clear_flag(row->bar_marker, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(row->bar_marker, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(row->bar_marker, 2, LV_PART_MAIN);
            lv_obj_add_flag(row->bar_marker, LV_OBJ_FLAG_HIDDEN);

            row->high_label = lv_label_create(row->container);
            lv_label_set_text(row->high_label, "--");
            lv_obj_set_style_text_align(row->high_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        }
    }

#if APP_UI_WEATHER_LOTTIE_ENABLED
    lv_obj_t *lottie_icon = lv_lottie_create(card);
    lv_obj_set_style_bg_opa(lottie_icon, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(lottie_icon, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(lottie_icon, 0, LV_PART_MAIN);
    lv_obj_add_flag(lottie_icon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(lottie_icon, LV_OBJ_FLAG_SCROLLABLE);
    ctx->lottie_icon = lottie_icon;
#endif

    lv_obj_add_event_cb(card, w_weather_tile_event_cb, LV_EVENT_DELETE, ctx);
    out_instance->ctx = ctx;
    out_instance->obj = card;
    weather_render(card, ctx, NULL, false);
    return ESP_OK;
}

void w_weather_tile_apply_state(ui_widget_instance_t *instance, const ha_state_t *state)
{
    if (instance == NULL || instance->obj == NULL || state == NULL) {
        return;
    }

    /* Weather tile is driven by its primary weather entity only.
     * Secondary entity updates (if configured) must not override icon/condition rendering. */
    if (strncmp(state->entity_id, instance->entity_id, APP_MAX_ENTITY_ID_LEN) != 0) {
        return;
    }

    w_weather_tile_ctx_t *ctx = (w_weather_tile_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }

    /* Deterministic icon behavior:
     * As soon as a weather condition maps to an icon, keep showing that icon
     * until a new valid weather condition arrives. */
    weather_update_icon_cache_from_state(ctx, state->state);

    weather_values_t values = {0};
    weather_extract_values(state, ctx->show_forecast, &values);
    if (ctx->last_condition_text[0] == '\0' && weather_has_alpha(values.condition)) {
        weather_copy_text(ctx->last_condition_text, sizeof(ctx->last_condition_text), values.condition);
    }
    weather_render(instance->obj, ctx, &values, true);
}

void w_weather_tile_mark_unavailable(ui_widget_instance_t *instance)
{
    if (instance == NULL || instance->obj == NULL) {
        return;
    }
    w_weather_tile_ctx_t *ctx = (w_weather_tile_ctx_t *)instance->ctx;
    if (ctx == NULL) {
        return;
    }
    weather_render(instance->obj, ctx, NULL, false);
}
