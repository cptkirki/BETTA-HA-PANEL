#pragma once

#include "esp_err.h"

esp_err_t ui_i18n_init(const char *language_code);
void ui_i18n_reset(void);
const char *ui_i18n_get(const char *key, const char *fallback);
const char *ui_i18n_get_language(void);
