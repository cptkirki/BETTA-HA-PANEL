#include "layout/layout_validate.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"

#include "app_config.h"

static bool str_in_list(const char *value, const void *list, size_t entry_size, size_t list_len)
{
    if (value == NULL || list == NULL || entry_size == 0) {
        return false;
    }
    const char *base = (const char *)list;
    for (size_t i = 0; i < list_len; i++) {
        const char *entry = base + (i * entry_size);
        if (strncmp(value, entry, entry_size) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_valid_entity_id(const char *entity_id)
{
    if (entity_id == NULL) {
        return false;
    }

    size_t len = strlen(entity_id);
    if (len < 3 || len >= APP_MAX_ENTITY_ID_LEN) {
        return false;
    }

    const char *dot = strchr(entity_id, '.');
    if (dot == NULL || dot == entity_id || *(dot + 1) == '\0') {
        return false;
    }
    if (strchr(dot + 1, '.') != NULL) {
        return false;
    }

    for (const char *p = entity_id; *p != '\0'; p++) {
        if (*p == '.') {
            continue;
        }
        if (!(islower((unsigned char)*p) || isdigit((unsigned char)*p) || *p == '_')) {
            return false;
        }
    }

    return true;
}

static bool entity_in_domain(const char *entity_id, const char *domain)
{
    if (entity_id == NULL || domain == NULL) {
        return false;
    }
    size_t domain_len = strlen(domain);
    return strncmp(entity_id, domain, domain_len) == 0 && entity_id[domain_len] == '.';
}

static bool is_supported_widget_type(const char *type)
{
    if (type == NULL) {
        return false;
    }
    return (strcmp(type, "sensor") == 0) || (strcmp(type, "button") == 0) || (strcmp(type, "slider") == 0) ||
           (strcmp(type, "graph") == 0) || (strcmp(type, "light_tile") == 0) ||
           (strcmp(type, "heating_tile") == 0) || (strcmp(type, "weather_tile") == 0) ||
           (strcmp(type, "weather_3day") == 0);
}

typedef struct {
    int min_w;
    int min_h;
    int max_w;
    int max_h;
} widget_size_limits_t;

static widget_size_limits_t widget_size_limits_for_type(const char *type)
{
    widget_size_limits_t limits = {
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
        limits.min_w = 180;
        limits.min_h = 120;
        limits.max_w = 480;
        limits.max_h = 320;
    } else if (strcmp(type, "slider") == 0) {
        limits.min_w = 180;
        limits.min_h = 100;
    } else if (strcmp(type, "graph") == 0) {
        limits.min_w = 220;
        limits.min_h = 140;
    } else if (strcmp(type, "light_tile") == 0) {
        limits.min_w = 200;
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

static const char *required_domain_for_widget_type(const char *type)
{
    if (type == NULL) {
        return NULL;
    }
    if (strcmp(type, "sensor") == 0) {
        return "sensor";
    }
    if (strcmp(type, "light_tile") == 0) {
        return "light";
    }
    if (strcmp(type, "heating_tile") == 0) {
        return "climate";
    }
    if (strcmp(type, "weather_tile") == 0 || strcmp(type, "weather_3day") == 0) {
        return "weather";
    }
    return NULL;
}

static bool widget_entity_domain_valid(const char *type, const char *entity_id)
{
    if (type == NULL || entity_id == NULL) {
        return false;
    }

    if (strcmp(type, "sensor") == 0) {
        return entity_in_domain(entity_id, "sensor") || entity_in_domain(entity_id, "binary_sensor");
    }

    const char *required_domain = required_domain_for_widget_type(type);
    if (required_domain == NULL) {
        return true;
    }
    return entity_in_domain(entity_id, required_domain);
}

static bool is_hex_digit_char(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool is_valid_hex_rgb_color(const char *text)
{
    if (text == NULL || text[0] == '\0') {
        return false;
    }

    const char *p = text;
    if (p[0] == '#') {
        p++;
    } else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }

    if (strlen(p) != 6) {
        return false;
    }

    for (size_t i = 0; i < 6; i++) {
        if (!is_hex_digit_char(p[i])) {
            return false;
        }
    }
    return true;
}

static bool is_valid_slider_direction(const char *direction)
{
    if (direction == NULL || direction[0] == '\0') {
        return false;
    }
    return strcmp(direction, "auto") == 0 || strcmp(direction, "left_to_right") == 0 ||
           strcmp(direction, "right_to_left") == 0 || strcmp(direction, "bottom_to_top") == 0 ||
           strcmp(direction, "top_to_bottom") == 0;
}

void layout_validation_clear(layout_validation_result_t *result)
{
    if (result == NULL) {
        return;
    }
    result->count = 0;
    memset(result->messages, 0, sizeof(result->messages));
}

void layout_validation_add(layout_validation_result_t *result, const char *msg)
{
    if (result == NULL || msg == NULL) {
        return;
    }
    if (result->count >= APP_LAYOUT_MAX_ERRORS) {
        return;
    }
    snprintf(result->messages[result->count], sizeof(result->messages[result->count]), "%s", msg);
    result->count++;
}

static bool validate_widget(cJSON *widget, const char *known_widget_ids, size_t known_ids_len,
    size_t page_index, size_t widget_index, layout_validation_result_t *result)
{
    char msg[96];

    cJSON *id = cJSON_GetObjectItemCaseSensitive(widget, "id");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(widget, "type");
    cJSON *entity_id = cJSON_GetObjectItemCaseSensitive(widget, "entity_id");
    cJSON *secondary_entity_id = cJSON_GetObjectItemCaseSensitive(widget, "secondary_entity_id");
    cJSON *slider_direction = cJSON_GetObjectItemCaseSensitive(widget, "slider_direction");
    cJSON *slider_accent_color = cJSON_GetObjectItemCaseSensitive(widget, "slider_accent_color");
    cJSON *rect = cJSON_GetObjectItemCaseSensitive(widget, "rect");

    if (!cJSON_IsString(id) || id->valuestring == NULL || strlen(id->valuestring) == 0U) {
        snprintf(msg, sizeof(msg), "page[%u] widget[%u]: invalid id", (unsigned)page_index, (unsigned)widget_index);
        layout_validation_add(result, msg);
    } else if (strlen(id->valuestring) >= APP_MAX_WIDGET_ID_LEN) {
        snprintf(msg, sizeof(msg), "widget id too long: %s", id->valuestring);
        layout_validation_add(result, msg);
    } else if (str_in_list(id->valuestring, known_widget_ids, APP_MAX_WIDGET_ID_LEN, known_ids_len)) {
        snprintf(msg, sizeof(msg), "duplicate widget id: %s", id->valuestring);
        layout_validation_add(result, msg);
    }

    if (!cJSON_IsString(type) || type->valuestring == NULL) {
        snprintf(msg, sizeof(msg), "widget %s: missing type", cJSON_IsString(id) ? id->valuestring : "?");
        layout_validation_add(result, msg);
    } else if (!is_supported_widget_type(type->valuestring)) {
        snprintf(msg, sizeof(msg), "widget %s: unsupported type %s", cJSON_IsString(id) ? id->valuestring : "?",
            type->valuestring);
        layout_validation_add(result, msg);
    }

    if (!cJSON_IsString(entity_id) || !is_valid_entity_id(entity_id->valuestring)) {
        snprintf(msg, sizeof(msg), "widget %s: invalid entity_id", cJSON_IsString(id) ? id->valuestring : "?");
        layout_validation_add(result, msg);
    }

    if (cJSON_IsString(type) && type->valuestring != NULL && cJSON_IsString(entity_id) && entity_id->valuestring != NULL) {
        if (!widget_entity_domain_valid(type->valuestring, entity_id->valuestring)) {
            if (strcmp(type->valuestring, "sensor") == 0) {
                snprintf(msg, sizeof(msg), "widget %s: entity_id must be sensor.* or binary_sensor.*",
                    cJSON_IsString(id) ? id->valuestring : "?");
            } else {
                const char *required_domain = required_domain_for_widget_type(type->valuestring);
                snprintf(msg, sizeof(msg), "widget %s: entity_id must be %s.*",
                    cJSON_IsString(id) ? id->valuestring : "?", required_domain != NULL ? required_domain : "?");
            }
            layout_validation_add(result, msg);
        }
    }

    if (cJSON_IsString(type) && type->valuestring != NULL && strcmp(type->valuestring, "heating_tile") == 0) {
        if (cJSON_IsString(secondary_entity_id) && secondary_entity_id->valuestring != NULL &&
            secondary_entity_id->valuestring[0] != '\0') {
            if (!is_valid_entity_id(secondary_entity_id->valuestring) ||
                !entity_in_domain(secondary_entity_id->valuestring, "sensor")) {
                snprintf(msg, sizeof(msg), "widget %s: invalid secondary_entity_id", cJSON_IsString(id) ? id->valuestring : "?");
                layout_validation_add(result, msg);
            }
        }
    }

    if (cJSON_IsString(type) && type->valuestring != NULL && strcmp(type->valuestring, "slider") == 0) {
        if (slider_direction != NULL) {
            if (!cJSON_IsString(slider_direction) || slider_direction->valuestring == NULL ||
                !is_valid_slider_direction(slider_direction->valuestring)) {
                snprintf(msg, sizeof(msg),
                    "widget %s: slider_direction must be auto|left_to_right|right_to_left|bottom_to_top|top_to_bottom",
                    cJSON_IsString(id) ? id->valuestring : "?");
                layout_validation_add(result, msg);
            }
        }

        if (slider_accent_color != NULL) {
            if (!cJSON_IsString(slider_accent_color) || slider_accent_color->valuestring == NULL ||
                !is_valid_hex_rgb_color(slider_accent_color->valuestring)) {
                snprintf(msg, sizeof(msg), "widget %s: slider_accent_color must be hex RGB",
                    cJSON_IsString(id) ? id->valuestring : "?");
                layout_validation_add(result, msg);
            }
        }
    }

    if (!cJSON_IsObject(rect)) {
        snprintf(msg, sizeof(msg), "widget %s: missing rect", cJSON_IsString(id) ? id->valuestring : "?");
        layout_validation_add(result, msg);
    } else {
        cJSON *x = cJSON_GetObjectItemCaseSensitive(rect, "x");
        cJSON *y = cJSON_GetObjectItemCaseSensitive(rect, "y");
        cJSON *w = cJSON_GetObjectItemCaseSensitive(rect, "w");
        cJSON *h = cJSON_GetObjectItemCaseSensitive(rect, "h");
        if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y) || !cJSON_IsNumber(w) || !cJSON_IsNumber(h)) {
            snprintf(msg, sizeof(msg), "widget %s: rect values must be numbers",
                cJSON_IsString(id) ? id->valuestring : "?");
            layout_validation_add(result, msg);
        } else {
            int rx = x->valueint;
            int ry = y->valueint;
            int rw = w->valueint;
            int rh = h->valueint;
            widget_size_limits_t limits = widget_size_limits_for_type(cJSON_IsString(type) ? type->valuestring : NULL);
            if (rw <= 0 || rh <= 0 || rx < 0 || ry < 0 || (rx + rw) > APP_CONTENT_BOX_WIDTH ||
                (ry + rh) > APP_CONTENT_BOX_HEIGHT) {
                snprintf(msg, sizeof(msg), "widget %s: rect out of bounds for content box",
                    cJSON_IsString(id) ? id->valuestring : "?");
                layout_validation_add(result, msg);
            } else if (rw < limits.min_w || rh < limits.min_h || rw > limits.max_w || rh > limits.max_h) {
                snprintf(msg, sizeof(msg), "widget %s: size must be %dx%d..%dx%d",
                    cJSON_IsString(id) ? id->valuestring : "?",
                    limits.min_w,
                    limits.min_h,
                    limits.max_w,
                    limits.max_h);
                layout_validation_add(result, msg);
            }
        }
    }

    return true;
}

bool layout_validate_json(const char *json, layout_validation_result_t *result)
{
    layout_validation_clear(result);
    if (json == NULL) {
        layout_validation_add(result, "layout json is null");
        return false;
    }

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        layout_validation_add(result, "layout json parse error");
        return false;
    }

    cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    cJSON *pages = cJSON_GetObjectItemCaseSensitive(root, "pages");

    if (!cJSON_IsNumber(version) || version->valueint != 1) {
        layout_validation_add(result, "layout version must be 1");
    }

    if (!cJSON_IsArray(pages)) {
        layout_validation_add(result, "pages must be an array");
        cJSON_Delete(root);
        return false;
    }

    int page_count = cJSON_GetArraySize(pages);
    if (page_count <= 0) {
        layout_validation_add(result, "at least one page required");
    }
    if (page_count > APP_MAX_PAGES) {
        layout_validation_add(result, "too many pages");
    }

    char *known_page_ids = calloc(APP_MAX_PAGES, APP_MAX_PAGE_ID_LEN);
    char *known_widget_ids = calloc(APP_MAX_WIDGETS_TOTAL, APP_MAX_WIDGET_ID_LEN);
    if (known_page_ids == NULL || known_widget_ids == NULL) {
        free(known_page_ids);
        free(known_widget_ids);
        layout_validation_add(result, "out of memory during validation");
        cJSON_Delete(root);
        return false;
    }

    size_t known_page_ids_len = 0;
    size_t known_widget_ids_len = 0;

    for (int i = 0; i < page_count; i++) {
        cJSON *page = cJSON_GetArrayItem(pages, i);
        if (!cJSON_IsObject(page)) {
            layout_validation_add(result, "page entry must be object");
            continue;
        }

        cJSON *page_id = cJSON_GetObjectItemCaseSensitive(page, "id");
        cJSON *widgets = cJSON_GetObjectItemCaseSensitive(page, "widgets");
        char msg[96];

        if (!cJSON_IsString(page_id) || page_id->valuestring == NULL || strlen(page_id->valuestring) == 0U) {
            snprintf(msg, sizeof(msg), "page[%u]: invalid id", (unsigned)i);
            layout_validation_add(result, msg);
        } else if (strlen(page_id->valuestring) >= APP_MAX_PAGE_ID_LEN) {
            snprintf(msg, sizeof(msg), "page id too long: %s", page_id->valuestring);
            layout_validation_add(result, msg);
        } else if (str_in_list(page_id->valuestring, known_page_ids, APP_MAX_PAGE_ID_LEN, known_page_ids_len)) {
            snprintf(msg, sizeof(msg), "duplicate page id: %s", page_id->valuestring);
            layout_validation_add(result, msg);
        } else if (known_page_ids_len < APP_MAX_PAGES) {
            char *dst = known_page_ids + (known_page_ids_len * APP_MAX_PAGE_ID_LEN);
            snprintf(dst, APP_MAX_PAGE_ID_LEN, "%s", page_id->valuestring);
            known_page_ids_len++;
        }

        if (!cJSON_IsArray(widgets)) {
            snprintf(msg, sizeof(msg), "page %s: widgets must be array", cJSON_IsString(page_id) ? page_id->valuestring : "?");
            layout_validation_add(result, msg);
            continue;
        }

        int widget_count = cJSON_GetArraySize(widgets);
        if (widget_count > APP_MAX_WIDGETS_PER_PAGE) {
            snprintf(msg, sizeof(msg), "page %s: too many widgets", cJSON_IsString(page_id) ? page_id->valuestring : "?");
            layout_validation_add(result, msg);
        }

        for (int w = 0; w < widget_count; w++) {
            cJSON *widget = cJSON_GetArrayItem(widgets, w);
            if (!cJSON_IsObject(widget)) {
                layout_validation_add(result, "widget entry must be object");
                continue;
            }
            validate_widget(widget, known_widget_ids, known_widget_ids_len, (size_t)i, (size_t)w, result);
            cJSON *id = cJSON_GetObjectItemCaseSensitive(widget, "id");
            if (cJSON_IsString(id) && id->valuestring != NULL && strlen(id->valuestring) > 0 &&
                strlen(id->valuestring) < APP_MAX_WIDGET_ID_LEN &&
                !str_in_list(id->valuestring, known_widget_ids, APP_MAX_WIDGET_ID_LEN, known_widget_ids_len) &&
                known_widget_ids_len < APP_MAX_WIDGETS_TOTAL) {
                char *dst = known_widget_ids + (known_widget_ids_len * APP_MAX_WIDGET_ID_LEN);
                snprintf(dst, APP_MAX_WIDGET_ID_LEN, "%s", id->valuestring);
                known_widget_ids_len++;
            }
        }
    }

    free(known_page_ids);
    free(known_widget_ids);
    cJSON_Delete(root);
    return result->count == 0;
}
