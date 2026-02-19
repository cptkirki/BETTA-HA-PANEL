/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#include "settings/i18n_store.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *I18N_BUILTIN_DE =
    "{\"lvgl\":{"
    "\"common\":{\"on\":\"AN\",\"off\":\"AUS\",\"unavailable\":\"nicht verfuegbar\",\"paused\":\"pausiert\",\"playing\":\"spielt\"},"
    "\"topbar\":{\"ha\":\"HA\",\"ap\":\"AP\"},"
    "\"sensor\":{\"age\":{\"just_now\":\"gerade eben\",\"min_one\":\"vor 1 Min\",\"min_many\":\"vor %d Min\",\"hour_one\":\"vor 1 Std\",\"hour_many\":\"vor %d Std\",\"day_one\":\"vor 1 Tag\",\"day_many\":\"vor %d Tagen\"}},"
    "\"heating\":{\"target_format\":\"Soll %.1f C\",\"active\":\"Heizen aktiv\"},"
    "\"weather\":{\"unavailable\":\"Nicht verfuegbar\",\"humidity_format\":\"Luftfeuchte %d%%\"},"
    "\"graph\":{\"no_history\":\"keine Historie\",\"no_data\":\"keine Daten\",\"min\":\"min\",\"max\":\"max\"},"
    "\"boot\":{\"initializing_system\":\"System wird initialisiert\",\"initializing_wifi\":\"WLAN wird initialisiert\",\"initializing_touch\":\"Touch wird initialisiert\",\"wifi_setup_title\":\"WLAN Setup\",\"wifi_connect_failed\":\"WLAN Verbindung fehlgeschlagen\",\"wifi_credentials_missing\":\"WLAN Zugangsdaten fehlen\",\"open_editor\":\"BETTA Editor oeffnen:\",\"ha_setup_title\":\"Home Assistant Setup\",\"wifi_connected\":\"WLAN verbunden\",\"ha_credentials_missing\":\"HA Zugangsdaten fehlen\",\"set_ha_url_token\":\"HA URL und Token setzen\",\"loading_dashboard\":\"Dashboard wird geladen\",\"setup_ap_prefix\":\"Setup AP\",\"offline_mode\":\"Offline Modus\"}"
    "}}";

static const char *I18N_BUILTIN_EN =
    "{\"lvgl\":{"
    "\"common\":{\"on\":\"ON\",\"off\":\"OFF\",\"unavailable\":\"unavailable\",\"paused\":\"paused\",\"playing\":\"playing\"},"
    "\"topbar\":{\"ha\":\"HA\",\"ap\":\"AP\"},"
    "\"sensor\":{\"age\":{\"just_now\":\"just now\",\"min_one\":\"1 min ago\",\"min_many\":\"%d min ago\",\"hour_one\":\"1 hour ago\",\"hour_many\":\"%d hours ago\",\"day_one\":\"1 day ago\",\"day_many\":\"%d days ago\"}},"
    "\"heating\":{\"target_format\":\"Target %.1f C\",\"active\":\"heating active\"},"
    "\"weather\":{\"unavailable\":\"Unavailable\",\"humidity_format\":\"Humidity %d%%\"},"
    "\"graph\":{\"no_history\":\"no history\",\"no_data\":\"no data\",\"min\":\"min\",\"max\":\"max\"},"
    "\"boot\":{\"initializing_system\":\"Initializing system\",\"initializing_wifi\":\"Initializing Wi-Fi\",\"initializing_touch\":\"Initializing touch\",\"wifi_setup_title\":\"Wi-Fi Setup\",\"wifi_connect_failed\":\"Wi-Fi connect failed\",\"wifi_credentials_missing\":\"Wi-Fi credentials missing\",\"open_editor\":\"Open BETTA Editor:\",\"ha_setup_title\":\"Home Assistant Setup\",\"wifi_connected\":\"Wi-Fi connected\",\"ha_credentials_missing\":\"HA credentials missing\",\"set_ha_url_token\":\"Set HA URL and token\",\"loading_dashboard\":\"Loading dashboard\",\"setup_ap_prefix\":\"Setup AP\",\"offline_mode\":\"Offline mode\"}"
    "}}";

static const char *I18N_BUILTIN_ES =
    "{\"lvgl\":{"
    "\"common\":{\"on\":\"ENC\",\"off\":\"APAG\",\"unavailable\":\"no disponible\",\"paused\":\"pausado\",\"playing\":\"reproduciendo\"},"
    "\"topbar\":{\"ha\":\"HA\",\"ap\":\"AP\"},"
    "\"sensor\":{\"age\":{\"just_now\":\"ahora mismo\",\"min_one\":\"hace 1 min\",\"min_many\":\"hace %d min\",\"hour_one\":\"hace 1 hora\",\"hour_many\":\"hace %d horas\",\"day_one\":\"hace 1 dia\",\"day_many\":\"hace %d dias\"}},"
    "\"heating\":{\"target_format\":\"Objetivo %.1f C\",\"active\":\"calefaccion activa\"},"
    "\"weather\":{\"unavailable\":\"No disponible\",\"humidity_format\":\"Humedad %d%%\"},"
    "\"graph\":{\"no_history\":\"sin historial\",\"no_data\":\"sin datos\",\"min\":\"min\",\"max\":\"max\"},"
    "\"boot\":{\"initializing_system\":\"Inicializando sistema\",\"initializing_wifi\":\"Inicializando Wi-Fi\",\"initializing_touch\":\"Inicializando tactil\",\"wifi_setup_title\":\"Configuracion Wi-Fi\",\"wifi_connect_failed\":\"Error de conexion Wi-Fi\",\"wifi_credentials_missing\":\"Faltan credenciales Wi-Fi\",\"open_editor\":\"Abrir BETTA Editor:\",\"ha_setup_title\":\"Configuracion Home Assistant\",\"wifi_connected\":\"Wi-Fi conectado\",\"ha_credentials_missing\":\"Faltan credenciales HA\",\"set_ha_url_token\":\"Configurar URL y token de HA\",\"loading_dashboard\":\"Cargando panel\",\"setup_ap_prefix\":\"AP de configuracion\",\"offline_mode\":\"Modo sin conexion\"}"
    "}}";

static bool i18n_store_is_valid_language_code(const char *code)
{
    if (code == NULL) {
        return false;
    }

    size_t len = strlen(code);
    if (len < 2 || len >= APP_UI_LANGUAGE_MAX_LEN) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        char c = code[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) {
            return false;
        }
    }
    return true;
}

bool i18n_store_normalize_language_code(const char *input, char *out_code, size_t out_len)
{
    if (input == NULL || out_code == NULL || out_len == 0) {
        return false;
    }

    while (*input != '\0' && isspace((unsigned char)*input)) {
        input++;
    }

    size_t out = 0;
    while (input[out] != '\0' && !isspace((unsigned char)input[out])) {
        if (out + 1 >= out_len) {
            return false;
        }

        char c = (char)tolower((unsigned char)input[out]);
        out_code[out] = c;
        out++;
    }
    out_code[out] = '\0';

    if (out == 0) {
        return false;
    }
    return i18n_store_is_valid_language_code(out_code);
}

bool i18n_store_is_builtin_language(const char *language_code)
{
    if (language_code == NULL) {
        return false;
    }
    return strcmp(language_code, "de") == 0 || strcmp(language_code, "en") == 0 || strcmp(language_code, "es") == 0;
}

const char *i18n_store_builtin_translation_json(const char *language_code)
{
    if (language_code == NULL) {
        return NULL;
    }
    if (strcmp(language_code, "de") == 0) {
        return I18N_BUILTIN_DE;
    }
    if (strcmp(language_code, "es") == 0) {
        return I18N_BUILTIN_ES;
    }
    if (strcmp(language_code, "en") == 0) {
        return I18N_BUILTIN_EN;
    }
    return NULL;
}

static bool i18n_store_ensure_dir(void)
{
    struct stat st = {0};
    if (stat(APP_I18N_DIR, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }

    (void)mkdir(APP_I18N_DIR, 0775);
    if (stat(APP_I18N_DIR, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return false;
}

static bool i18n_store_build_path(const char *language_code, char *out_path, size_t out_path_len)
{
    if (language_code == NULL || out_path == NULL || out_path_len == 0) {
        return false;
    }
    int written = snprintf(out_path, out_path_len, "%s/%s.json", APP_I18N_DIR, language_code);
    return written > 0 && (size_t)written < out_path_len;
}

esp_err_t i18n_store_load_custom_translation(const char *language_code, char **out_json)
{
    if (language_code == NULL || out_json == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_json = NULL;

    char lang[APP_UI_LANGUAGE_MAX_LEN] = {0};
    if (!i18n_store_normalize_language_code(language_code, lang, sizeof(lang))) {
        return ESP_ERR_INVALID_ARG;
    }

    char path[96] = {0};
    if (!i18n_store_build_path(lang, path, sizeof(path))) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return ESP_FAIL;
    }

    long size = ftell(f);
    if (size <= 0 || (size_t)size > APP_I18N_MAX_JSON_LEN) {
        fclose(f);
        return ESP_ERR_INVALID_SIZE;
    }
    rewind(f);

    char *buf = calloc((size_t)size + 1U, sizeof(char));
    if (buf == NULL) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    size_t read = fread(buf, 1U, (size_t)size, f);
    fclose(f);
    if (read != (size_t)size) {
        free(buf);
        return ESP_FAIL;
    }

    *out_json = buf;
    return ESP_OK;
}

esp_err_t i18n_store_save_custom_translation(const char *language_code, const char *json_payload, size_t payload_len)
{
    if (language_code == NULL || json_payload == NULL || payload_len == 0 || payload_len > APP_I18N_MAX_JSON_LEN) {
        return ESP_ERR_INVALID_ARG;
    }

    char lang[APP_UI_LANGUAGE_MAX_LEN] = {0};
    if (!i18n_store_normalize_language_code(language_code, lang, sizeof(lang))) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!i18n_store_ensure_dir()) {
        return ESP_FAIL;
    }

    char path[96] = {0};
    if (!i18n_store_build_path(lang, path, sizeof(path))) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        return ESP_FAIL;
    }
    size_t written = fwrite(json_payload, 1U, payload_len, f);
    fclose(f);
    if (written != payload_len) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool i18n_store_custom_translation_exists(const char *language_code)
{
    char lang[APP_UI_LANGUAGE_MAX_LEN] = {0};
    if (!i18n_store_normalize_language_code(language_code, lang, sizeof(lang))) {
        return false;
    }

    char path[96] = {0};
    if (!i18n_store_build_path(lang, path, sizeof(path))) {
        return false;
    }

    struct stat st = {0};
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static bool i18n_store_add_language(
    char (*out_codes)[APP_UI_LANGUAGE_MAX_LEN],
    size_t max_codes,
    size_t *count,
    const char *code)
{
    if (out_codes == NULL || count == NULL || code == NULL || code[0] == '\0') {
        return false;
    }
    for (size_t i = 0; i < *count; i++) {
        if (strncmp(out_codes[i], code, APP_UI_LANGUAGE_MAX_LEN) == 0) {
            return false;
        }
    }
    if (*count >= max_codes) {
        return false;
    }
    strlcpy(out_codes[*count], code, APP_UI_LANGUAGE_MAX_LEN);
    (*count)++;
    return true;
}

esp_err_t i18n_store_list_languages(
    char (*out_codes)[APP_UI_LANGUAGE_MAX_LEN],
    size_t max_codes,
    size_t *out_count)
{
    if (out_codes == NULL || max_codes == 0 || out_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t count = 0;
    (void)i18n_store_add_language(out_codes, max_codes, &count, "de");
    (void)i18n_store_add_language(out_codes, max_codes, &count, "en");
    (void)i18n_store_add_language(out_codes, max_codes, &count, "es");

    if (!i18n_store_ensure_dir()) {
        *out_count = count;
        return ESP_OK;
    }

    DIR *dir = opendir(APP_I18N_DIR);
    if (dir == NULL) {
        *out_count = count;
        return (errno == ENOENT) ? ESP_OK : ESP_FAIL;
    }

    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);
        if (len <= strlen(".json")) {
            continue;
        }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }
        if (strcmp(name + (len - 5), ".json") != 0) {
            continue;
        }

        char code[APP_UI_LANGUAGE_MAX_LEN] = {0};
        size_t base_len = len - 5;
        if (base_len >= sizeof(code)) {
            continue;
        }
        memcpy(code, name, base_len);
        code[base_len] = '\0';

        char normalized[APP_UI_LANGUAGE_MAX_LEN] = {0};
        if (!i18n_store_normalize_language_code(code, normalized, sizeof(normalized))) {
            continue;
        }

        (void)i18n_store_add_language(out_codes, max_codes, &count, normalized);
    }
    closedir(dir);

    *out_count = count;
    return ESP_OK;
}
