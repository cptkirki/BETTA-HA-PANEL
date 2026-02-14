#include "ui/ui_bindings.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ha/ha_client.h"
#include "ha/ha_services.h"

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
    char domain[32] = {0};
    if (!split_entity_id(entity_id, domain, sizeof(domain))) {
        return ESP_ERR_INVALID_ARG;
    }
    char payload[160] = {0};
    snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\"}", entity_id);
    return ha_client_call_service(domain, HA_SERVICE_TOGGLE, payload);
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
        snprintf(payload, sizeof(payload), "{\"entity_id\":\"%s\",\"brightness\":%d}", entity_id, brightness);
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
