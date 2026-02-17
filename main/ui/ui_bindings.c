#include "ui/ui_bindings.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#include "esp_timer.h"
#include "esp_log.h"

#include "app_config.h"
#include "app_events.h"
#include "ha/ha_client.h"
#include "ha/ha_model.h"
#include "ha/ha_services.h"

#define UI_BINDINGS_POWER_CMD_DEBOUNCE_MS 250
#define UI_BINDINGS_CMD_DEBOUNCE_SLOTS 24
static const char *TAG = "ui_bindings";

typedef struct {
    bool used;
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    int64_t last_cmd_ms;
    bool last_target_known;
    bool last_target_on;
} ui_bindings_cmd_debounce_t;

static ui_bindings_cmd_debounce_t s_power_cmd_debounce[UI_BINDINGS_CMD_DEBOUNCE_SLOTS];

static void ui_bindings_publish_state_changed_event(const char *entity_id)
{
    if (entity_id == NULL || entity_id[0] == '\0') {
        return;
    }

    app_event_t event = {.type = EV_HA_STATE_CHANGED};
    strlcpy(event.data.ha_state_changed.entity_id, entity_id, sizeof(event.data.ha_state_changed.entity_id));
    if (!app_events_publish(&event, pdMS_TO_TICKS(5))) {
        ESP_LOGW(TAG, "failed to enqueue optimistic state event for %s", entity_id);
    } else {
#if APP_HA_ROUTE_TRACE_LOG
        ESP_LOGI(TAG, "route panel_touch->panel entity=%s source=optimistic", entity_id);
#endif
    }
}

static void ui_bindings_apply_optimistic_power_state(const char *entity_id, bool on)
{
    if (entity_id == NULL || entity_id[0] == '\0') {
        return;
    }

    ha_state_t state = {0};
    if (!ha_model_get_state(entity_id, &state)) {
        strlcpy(state.entity_id, entity_id, sizeof(state.entity_id));
        strlcpy(state.attributes_json, "{}", sizeof(state.attributes_json));
    }

    strlcpy(state.state, on ? "on" : "off", sizeof(state.state));
    state.last_changed_unix_ms = esp_timer_get_time() / 1000;
    if (ha_model_upsert_state(&state) == ESP_OK) {
        ui_bindings_publish_state_changed_event(entity_id);
    }
}

static void ui_bindings_apply_optimistic_state_text(const char *entity_id, const char *state_text)
{
    if (entity_id == NULL || entity_id[0] == '\0' || state_text == NULL || state_text[0] == '\0') {
        return;
    }

    ha_state_t state = {0};
    if (!ha_model_get_state(entity_id, &state)) {
        strlcpy(state.entity_id, entity_id, sizeof(state.entity_id));
        strlcpy(state.attributes_json, "{}", sizeof(state.attributes_json));
    }

    strlcpy(state.state, state_text, sizeof(state.state));
    state.last_changed_unix_ms = esp_timer_get_time() / 1000;
    if (ha_model_upsert_state(&state) == ESP_OK) {
        ui_bindings_publish_state_changed_event(entity_id);
    }
}

static bool ui_bindings_allow_power_command_now(const char *entity_id, bool target_known, bool target_on)
{
    if (entity_id == NULL || entity_id[0] == '\0') {
        return false;
    }

    int64_t now_ms = esp_timer_get_time() / 1000;
    int free_idx = -1;
    int oldest_idx = 0;
    int64_t oldest_ts = INT64_MAX;

    for (int i = 0; i < UI_BINDINGS_CMD_DEBOUNCE_SLOTS; i++) {
        if (!s_power_cmd_debounce[i].used) {
            if (free_idx < 0) {
                free_idx = i;
            }
            continue;
        }

        if (strncmp(s_power_cmd_debounce[i].entity_id, entity_id, APP_MAX_ENTITY_ID_LEN) == 0) {
            int64_t age_ms = now_ms - s_power_cmd_debounce[i].last_cmd_ms;
            bool duplicate_target =
                target_known &&
                s_power_cmd_debounce[i].last_target_known &&
                (s_power_cmd_debounce[i].last_target_on == target_on);
            if (duplicate_target && age_ms < UI_BINDINGS_POWER_CMD_DEBOUNCE_MS) {
                ESP_LOGD(TAG, "drop duplicate power cmd entity=%s target=%s age=%" PRId64 "ms",
                    entity_id, target_on ? "on" : "off", age_ms);
                return false;
            }
            s_power_cmd_debounce[i].last_cmd_ms = now_ms;
            s_power_cmd_debounce[i].last_target_known = target_known;
            s_power_cmd_debounce[i].last_target_on = target_on;
            return true;
        }

        if (s_power_cmd_debounce[i].last_cmd_ms < oldest_ts) {
            oldest_ts = s_power_cmd_debounce[i].last_cmd_ms;
            oldest_idx = i;
        }
    }

    int slot = (free_idx >= 0) ? free_idx : oldest_idx;
    s_power_cmd_debounce[slot].used = true;
    s_power_cmd_debounce[slot].last_cmd_ms = now_ms;
    s_power_cmd_debounce[slot].last_target_known = target_known;
    s_power_cmd_debounce[slot].last_target_on = target_on;
    strlcpy(s_power_cmd_debounce[slot].entity_id, entity_id, sizeof(s_power_cmd_debounce[slot].entity_id));
    return true;
}

static bool split_entity_id(const char *entity_id, char *domain_out, size_t domain_len)
{
    if (entity_id == NULL || domain_out == NULL || domain_len == 0) {
        return false;
    }
    const char *dot = strchr(entity_id, '.');
    if (dot == NULL || dot == entity_id) {
        return false;
    }
    size_t len = (size_t)(dot - entity_id);
    if (len >= domain_len) {
        len = domain_len - 1U;
    }
    memcpy(domain_out, entity_id, len);
    domain_out[len] = '\0';
    return true;
}

esp_err_t ui_bindings_toggle_entity(const char *entity_id)
{
    if (entity_id == NULL || entity_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    bool target_known = false;
    bool target_on = false;
    ha_state_t current = {0};
    if (ha_model_get_state(entity_id, &current)) {
        if (strcmp(current.state, "on") == 0) {
            target_known = true;
            target_on = false;
        } else if (strcmp(current.state, "off") == 0) {
            target_known = true;
            target_on = true;
        }
    }

    if (!ui_bindings_allow_power_command_now(entity_id, target_known, target_on)) {
        return ESP_OK;
    }
    char domain[32] = {0};
    if (!split_entity_id(entity_id, domain, sizeof(domain))) {
        return ESP_ERR_INVALID_ARG;
    }
    char payload[192] = {0};
    if (strcmp(domain, HA_DOMAIN_LIGHT) == 0) {
#if APP_HA_LIGHT_USE_TRANSITION_ZERO
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\",\"transition\":0}", entity_id);
#else
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\"}", entity_id);
#endif
    } else {
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\"}", entity_id);
    }

    const char *service = HA_SERVICE_TOGGLE;
    if (target_known) {
        service = target_on ? HA_SERVICE_TURN_ON : HA_SERVICE_TURN_OFF;
    }

    bool optimistic_on = target_known ? target_on : true;

    esp_err_t err = ha_client_call_service(domain, service, payload);
    if (err == ESP_OK) {
        ui_bindings_apply_optimistic_power_state(entity_id, optimistic_on);
    } else {
        ESP_LOGW(TAG, "toggle failed entity=%s service=%s err=%s", entity_id, service, esp_err_to_name(err));
    }
    return err;
}

esp_err_t ui_bindings_set_entity_power(const char *entity_id, bool on)
{
    if (entity_id == NULL || entity_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    char domain[32] = {0};
    if (!split_entity_id(entity_id, domain, sizeof(domain))) {
        return ESP_ERR_INVALID_ARG;
    }
    bool is_light = (strcmp(domain, HA_DOMAIN_LIGHT) == 0);

#if APP_HA_LIGHT_POWER_USE_TOGGLE
    bool current_known = false;
    bool current_on = false;
    if (is_light) {
        ha_state_t current = {0};
        if (ha_model_get_state(entity_id, &current)) {
            if (strcmp(current.state, "on") == 0) {
                current_known = true;
                current_on = true;
            } else if (strcmp(current.state, "off") == 0) {
                current_known = true;
                current_on = false;
            }
        }
    }
    if (current_known && (current_on == on)) {
        return ESP_OK;
    }
#endif

    if (!ui_bindings_allow_power_command_now(entity_id, true, on)) {
        return ESP_OK;
    }

    const char *service = on ? HA_SERVICE_TURN_ON : HA_SERVICE_TURN_OFF;
#if APP_HA_LIGHT_POWER_USE_TOGGLE
    if (is_light && current_known) {
        service = HA_SERVICE_TOGGLE;
    }
#endif

    char payload[192] = {0};
    if (is_light) {
#if APP_HA_LIGHT_USE_TRANSITION_ZERO
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\",\"transition\":0}", entity_id);
#else
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\"}", entity_id);
#endif
    } else {
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\"}", entity_id);
    }
    esp_err_t err = ha_client_call_service(domain, service, payload);
    if (err == ESP_OK) {
        ui_bindings_apply_optimistic_power_state(entity_id, on);
    } else {
        ESP_LOGW(TAG, "set power failed entity=%s service=%s err=%s", entity_id, service, esp_err_to_name(err));
    }
    return err;
}

esp_err_t ui_bindings_set_slider_value(const char *entity_id, int value)
{
    if (entity_id == NULL || entity_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (value < 0) {
        value = 0;
    }
    if (value > 100) {
        value = 100;
    }

    char domain[32] = {0};
    if (!split_entity_id(entity_id, domain, sizeof(domain))) {
        return ESP_ERR_INVALID_ARG;
    }

    char payload[256] = {0};
    const char *service = HA_SERVICE_SET_VALUE;

    if (strcmp(domain, HA_DOMAIN_LIGHT) == 0) {
        int brightness = (value * 255) / 100;
        service = HA_SERVICE_TURN_ON;
#if APP_HA_LIGHT_USE_TRANSITION_ZERO
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\",\"brightness\":%d,\"transition\":0}", entity_id,
            brightness);
#else
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\",\"brightness\":%d}", entity_id, brightness);
#endif
    } else if (strcmp(domain, HA_DOMAIN_MEDIA_PLAYER) == 0) {
        service = "volume_set";
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\",\"volume_level\":%.2f}", entity_id, (float)value / 100.0f);
    } else if (strcmp(domain, HA_DOMAIN_CLIMATE) == 0) {
        service = "set_temperature";
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\",\"temperature\":%d}", entity_id, value);
    } else {
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\",\"value\":%d}", entity_id, value);
    }

    return ha_client_call_service(domain, service, payload);
}

esp_err_t ui_bindings_media_player_action(const char *entity_id, ui_bindings_media_action_t action)
{
    if (entity_id == NULL || entity_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    char domain[32] = {0};
    if (!split_entity_id(entity_id, domain, sizeof(domain))) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strcmp(domain, HA_DOMAIN_MEDIA_PLAYER) != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *service = NULL;
    switch (action) {
    case UI_BINDINGS_MEDIA_ACTION_PLAY_PAUSE:
        service = "media_play_pause";
        break;
    case UI_BINDINGS_MEDIA_ACTION_STOP:
        service = "media_stop";
        break;
    case UI_BINDINGS_MEDIA_ACTION_NEXT:
        service = "media_next_track";
        break;
    case UI_BINDINGS_MEDIA_ACTION_PREVIOUS:
        service = "media_previous_track";
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    char payload[192] = {0};
    snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\"}", entity_id);

    esp_err_t err = ha_client_call_service(domain, service, payload);
    if (err == ESP_OK && action == UI_BINDINGS_MEDIA_ACTION_PLAY_PAUSE) {
        ha_state_t current = {0};
        if (ha_model_get_state(entity_id, &current) && strcmp(current.state, "playing") == 0) {
            ui_bindings_apply_optimistic_state_text(entity_id, "paused");
        } else {
            ui_bindings_apply_optimistic_state_text(entity_id, "playing");
        }
    }
    return err;
}
