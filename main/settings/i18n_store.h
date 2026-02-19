/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#include "app_config.h"

bool i18n_store_normalize_language_code(const char *input, char *out_code, size_t out_len);
bool i18n_store_is_builtin_language(const char *language_code);
const char *i18n_store_builtin_translation_json(const char *language_code);

esp_err_t i18n_store_load_custom_translation(const char *language_code, char **out_json);
esp_err_t i18n_store_save_custom_translation(const char *language_code, const char *json_payload, size_t payload_len);
bool i18n_store_custom_translation_exists(const char *language_code);
esp_err_t i18n_store_list_languages(
    char (*out_codes)[APP_UI_LANGUAGE_MAX_LEN],
    size_t max_codes,
    size_t *out_count);
