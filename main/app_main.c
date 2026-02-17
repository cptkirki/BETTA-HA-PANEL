#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "soc/soc_caps.h"

#include "api/http_server.h"
#include "app_config.h"
#include "app_events.h"
#include "drivers/display_init.h"
#include "drivers/touch_init.h"
#include "ha/ha_client.h"
#include "ha/ha_model.h"
#include "layout/layout_store.h"
#include "net/time_sync.h"
#include "net/wifi_mgr.h"
#include "settings/runtime_settings.h"
#include "ui/ui_boot_splash.h"
#include "ui/ui_i18n.h"
#include "ui/ui_runtime.h"
#include "util/log_tags.h"

#if CONFIG_IDF_TARGET_ESP32P4 && CONFIG_ESP32P4_SELECTS_REV_LESS_V3 && (CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ > 360)
#error "ESP32-P4 rev<3 supports up to 360 MHz in this IDF; lower CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ."
#endif

static runtime_settings_t s_runtime_settings = {0};

typedef enum {
    BOOT_SCREEN_DASHBOARD = 0,
    BOOT_SCREEN_WIFI_SETUP = 1,
    BOOT_SCREEN_HA_SETUP = 2,
} boot_screen_mode_t;

static void app_show_wifi_setup_screen(bool had_wifi_credentials)
{
    char ap_ip[16] = "192.168.4.1";
    if (wifi_mgr_get_ap_ip(ap_ip, sizeof(ap_ip)) != ESP_OK) {
        strlcpy(ap_ip, "192.168.4.1", sizeof(ap_ip));
    }

    const char *ap_ssid = wifi_mgr_get_setup_ap_ssid();
    if (ap_ssid == NULL || ap_ssid[0] == '\0') {
        ap_ssid = APP_SETUP_AP_SSID_PREFIX;
    }

    char ssid_line[64] = {0};
    char url_line[64] = {0};
    snprintf(ssid_line, sizeof(ssid_line), "%s: %s", ui_i18n_get("topbar.ap", "AP"), ap_ssid);
    snprintf(url_line, sizeof(url_line), "http://%s", ap_ip);

    ui_boot_splash_set_title(ui_i18n_get("boot.wifi_setup_title", "Wi-Fi Setup"));
    ui_boot_splash_set_status_layout(true, 520, 0);
    ui_boot_splash_clear_status();
    ui_boot_splash_set_progress(100);
    ui_boot_splash_set_status(
        had_wifi_credentials ? ui_i18n_get("boot.wifi_connect_failed", "Wi-Fi connect failed")
                             : ui_i18n_get("boot.wifi_credentials_missing", "Wi-Fi credentials missing"));
    ui_boot_splash_set_status(ssid_line);
    ui_boot_splash_set_status(ui_i18n_get("boot.open_editor", "Open BETTA Editor:"));
    ui_boot_splash_set_status(url_line);
}

static void app_show_ha_setup_screen(void)
{
    char sta_ip[16] = {0};
    char url_line[64] = {0};
    if (wifi_mgr_get_sta_ip(sta_ip, sizeof(sta_ip)) == ESP_OK && sta_ip[0] != '\0') {
        snprintf(url_line, sizeof(url_line), "http://%s", sta_ip);
    } else {
        snprintf(url_line, sizeof(url_line), "http://<panel-ip>");
    }

    ui_boot_splash_set_title(ui_i18n_get("boot.ha_setup_title", "Home Assistant Setup"));
    ui_boot_splash_set_status_layout(true, 520, 0);
    ui_boot_splash_clear_status();
    ui_boot_splash_set_progress(100);
    ui_boot_splash_set_status(ui_i18n_get("boot.wifi_connected", "Wi-Fi connected"));
    ui_boot_splash_set_status(ui_i18n_get("boot.ha_credentials_missing", "HA credentials missing"));
    ui_boot_splash_set_status(ui_i18n_get("boot.open_editor", "Open BETTA Editor:"));
    ui_boot_splash_set_status(url_line);
    ui_boot_splash_set_status(ui_i18n_get("boot.set_ha_url_token", "Set HA URL and token"));
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static esp_err_t init_littlefs(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = NULL,
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    return esp_vfs_littlefs_register(&conf);
}

static esp_err_t init_net_stack(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG_APP, "Booting %s", APP_NAME);

    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(init_littlefs());
    ESP_ERROR_CHECK(init_net_stack());
    ESP_ERROR_CHECK(app_events_init());
    ESP_ERROR_CHECK(ha_model_init());
    ESP_ERROR_CHECK(runtime_settings_init());

    esp_err_t settings_err = runtime_settings_load(&s_runtime_settings);
    if (settings_err != ESP_OK) {
        ESP_LOGW(TAG_APP, "Failed to load runtime settings (%s), continuing with defaults", esp_err_to_name(settings_err));
        runtime_settings_set_defaults(&s_runtime_settings);
    }
    (void)ui_i18n_init(s_runtime_settings.ui_language);
    (void)time_sync_set_timezone(s_runtime_settings.time_tz);
    ESP_ERROR_CHECK(display_init());
    (void)ui_boot_splash_show();

    ui_boot_splash_set_status(ui_i18n_get("boot.initializing_wifi", "Initializing Wi-Fi"));

    bool has_wifi_credentials = false;
    bool wifi_ready = false;

#if SOC_WIFI_SUPPORTED || CONFIG_ESP_HOSTED_ENABLED
    has_wifi_credentials = runtime_settings_has_wifi(&s_runtime_settings);
    if (has_wifi_credentials) {
        wifi_mgr_config_t wifi_cfg = {
            .ssid = s_runtime_settings.wifi_ssid,
            .password = s_runtime_settings.wifi_password,
            .country_code = s_runtime_settings.wifi_country_code,
            .wait_for_ip = APP_WIFI_WAIT_FOR_IP,
            .connect_timeout_ms = APP_WIFI_CONNECT_TIMEOUT_MS,
            .max_retries = APP_WIFI_MAX_RETRIES,
        };
        esp_err_t wifi_err = wifi_mgr_init(&wifi_cfg);
        if (wifi_err != ESP_OK) {
            ESP_LOGW(TAG_WIFI, "Wi-Fi init failed: %s", esp_err_to_name(wifi_err));
            ui_boot_splash_set_status(ui_i18n_get("boot.wifi_connect_failed", "Wi-Fi connect failed"));
        } else {
            wifi_ready = true;
            time_sync_start(s_runtime_settings.ntp_server);
            time_sync_wait_for_sync(8000);
            ui_boot_splash_set_status(ui_i18n_get("boot.wifi_connected", "Wi-Fi connected"));
        }
    } else {
        ESP_LOGW(TAG_WIFI, "No Wi-Fi credentials configured, starting setup AP");
    }

    if (!wifi_ready) {
        wifi_mgr_ap_config_t ap_cfg = {
            .ssid = NULL,
            .password = APP_SETUP_AP_PASSWORD,
            .country_code = s_runtime_settings.wifi_country_code,
            .channel = APP_SETUP_AP_CHANNEL,
            .max_connection = APP_SETUP_AP_MAX_CONNECTIONS,
        };
        esp_err_t ap_err = wifi_mgr_start_setup_ap(&ap_cfg);
        if (ap_err == ESP_OK) {
            char ap_status[64] = {0};
            snprintf(
                ap_status,
                sizeof(ap_status),
                "%s: %s",
                ui_i18n_get("boot.setup_ap_prefix", "Setup AP"),
                wifi_mgr_get_setup_ap_ssid());
            ui_boot_splash_set_status(ap_status);
            ESP_LOGW(TAG_WIFI, "Setup AP started: %s", wifi_mgr_get_setup_ap_ssid());
        } else {
            ESP_LOGW(TAG_WIFI, "Failed to start setup AP: %s", esp_err_to_name(ap_err));
            ui_boot_splash_set_status(ui_i18n_get("boot.offline_mode", "Offline mode"));
        }
    }
#else
    ESP_LOGW(TAG_WIFI, "No Wi-Fi backend enabled for target %s", CONFIG_IDF_TARGET);
    ui_boot_splash_set_status("No Wi-Fi backend");
#endif

    boot_screen_mode_t boot_screen_mode = BOOT_SCREEN_DASHBOARD;
    if (!wifi_ready) {
        boot_screen_mode = BOOT_SCREEN_WIFI_SETUP;
    } else if (!runtime_settings_has_ha(&s_runtime_settings)) {
        boot_screen_mode = BOOT_SCREEN_HA_SETUP;
    }

    ESP_ERROR_CHECK(layout_store_init());
    ESP_ERROR_CHECK(http_server_start());

    if (boot_screen_mode == BOOT_SCREEN_DASHBOARD) {
        ui_boot_splash_set_status(ui_i18n_get("boot.initializing_touch", "Initializing touch"));
        esp_err_t touch_err = touch_init();
        if (touch_err != ESP_OK) {
            ESP_LOGW(TAG_TOUCH, "Touch init failed, continuing without touch input: %s", esp_err_to_name(touch_err));
        }

        ui_boot_splash_set_status(ui_i18n_get("boot.loading_dashboard", "Loading dashboard"));
        ESP_ERROR_CHECK(ui_runtime_init());
        ESP_ERROR_CHECK(ui_runtime_reload_layout());
        ESP_ERROR_CHECK(ui_runtime_start());
        ui_boot_splash_hide();

        ha_client_config_t ha_cfg = {
            .ws_url = s_runtime_settings.ha_ws_url,
            .access_token = s_runtime_settings.ha_access_token,
            .rest_enabled = s_runtime_settings.ha_rest_enabled,
        };
        esp_err_t ha_err = ha_client_start(&ha_cfg);
        if (ha_err != ESP_OK) {
            ESP_LOGW(TAG_HA_CLIENT, "HA client start failed: %s", esp_err_to_name(ha_err));
        }
    } else if (boot_screen_mode == BOOT_SCREEN_WIFI_SETUP) {
        app_show_wifi_setup_screen(has_wifi_credentials);
        ESP_LOGW(TAG_APP, "Provisioning screen active: Wi-Fi setup required");
    } else {
        app_show_ha_setup_screen();
        ESP_LOGW(TAG_HA_CLIENT, "HA settings missing, showing setup screen with web editor URL");
    }
}
