#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#include "app_config.h"

typedef struct {
    char id[APP_MAX_ENTITY_ID_LEN];
    char name[APP_MAX_NAME_LEN];
    char domain[APP_MAX_NAME_LEN];
    char unit[APP_MAX_UNIT_LEN];
    char device_class[APP_MAX_NAME_LEN];
    uint32_t supported_features;
    char icon[APP_MAX_ICON_LEN];
} ha_entity_info_t;

typedef struct {
    char entity_id[APP_MAX_ENTITY_ID_LEN];
    char state[APP_MAX_STATE_LEN];
    char attributes_json[APP_HA_ATTRS_MAX_LEN];
    int64_t last_changed_unix_ms;
} ha_state_t;

esp_err_t ha_model_init(void);
void ha_model_reset(void);
esp_err_t ha_model_upsert_entity(const ha_entity_info_t *entity);
esp_err_t ha_model_upsert_state(const ha_state_t *state);
bool ha_model_get_state(const char *entity_id, ha_state_t *out_state);
uint32_t ha_model_state_revision(void);
size_t ha_model_list_entities(
    const char *domain_filter, const char *search, ha_entity_info_t *out_entities, size_t max_out);
size_t ha_model_list_states(ha_state_t *out_states, size_t max_out);
