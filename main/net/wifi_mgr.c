#include "net/wifi_mgr.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "soc/soc_caps.h"

#if CONFIG_ESP_HOSTED_ENABLED
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_hosted.h"
#include "esp_system.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "app_config.h"
#include "ui/ui_boot_splash.h"
#include "util/log_tags.h"

static bool s_wifi_connected = false;
static EventGroupHandle_t s_wifi_event_group = NULL;
static esp_event_handler_instance_t s_wifi_event_instance = NULL;
static esp_event_handler_instance_t s_ip_event_instance = NULL;
static esp_netif_t *s_wifi_sta_netif = NULL;
static esp_netif_t *s_wifi_ap_netif = NULL;
static bool s_setup_ap_active = false;
static char s_setup_ap_ssid[APP_WIFI_SSID_MAX_LEN] = {0};
static bool s_wifi_scan_in_progress = false;
static wifi_config_t s_cached_sta_cfg = {0};
static bool s_cached_sta_cfg_valid = false;
static char s_wifi_country_code[APP_WIFI_COUNTRY_CODE_MAX_LEN] = {0};

#if CONFIG_ESP_HOSTED_ENABLED
static bool s_hosted_transport_ready = false;
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_SCAN_DONE_BIT BIT2
#define WIFI_STA_STARTED_BIT BIT3
#define WIFI_MAX_RETRIES_DEFAULT 8
#define WIFI_RECOVER_DISCONNECT_CONNECT_COOLDOWN_MS 5000
#define WIFI_RECOVER_STOP_START_COOLDOWN_MS 20000
#define WIFI_RECOVER_HARD_ATTEMPT_THRESHOLD 8
#define WIFI_RECOVER_DISC_CONN_PERIOD 4
#define WIFI_CONNECT_MIN_GAP_MS 1500
#define WIFI_RECONNECT_DELAY_MIN_MS 500
#define WIFI_RECONNECT_DELAY_HARD_MS 2000
#define WIFI_RECONNECT_DELAY_MAX_MS 20000
#define WIFI_SCAN_TIMEOUT_MS 8000
#define WIFI_STA_START_WAIT_MS 2000
#define WIFI_HOSTED_HARD_RECOVER_COOLDOWN_MS 20000

static int s_wifi_max_retries = WIFI_MAX_RETRIES_DEFAULT;
static int64_t s_last_recover_disc_conn_ms = 0;
static int64_t s_last_recover_stop_start_ms = 0;
static int64_t s_last_connect_request_ms = 0;
static int64_t s_last_hosted_hard_recover_ms = 0;
static esp_timer_handle_t s_reconnect_timer = NULL;
static uint8_t s_pending_reconnect_reason = 0;
static int s_pending_reconnect_attempt = 0;
static int s_wifi_scan_last_status = 0;

static esp_err_t wifi_mgr_force_reconnect_internal(bool allow_transport_escalation);

static void wifi_mgr_reset_reconnect_state(void)
{
    if (s_reconnect_timer != NULL && esp_timer_is_active(s_reconnect_timer)) {
        (void)esp_timer_stop(s_reconnect_timer);
    }
    s_pending_reconnect_attempt = 0;
    s_pending_reconnect_reason = 0;
}

static void wifi_mgr_set_setup_ap_state(bool active, const char *ssid)
{
    s_setup_ap_active = active;
    if (active && ssid != NULL) {
        strlcpy(s_setup_ap_ssid, ssid, sizeof(s_setup_ap_ssid));
    } else {
        s_setup_ap_ssid[0] = '\0';
    }
}

static void wifi_mgr_build_default_setup_ssid(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return;
    }

    uint8_t mac[6] = {0};
    esp_err_t mac_err = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (mac_err == ESP_OK) {
        snprintf(out, out_len, "%s-%02X%02X", APP_SETUP_AP_SSID_PREFIX, mac[4], mac[5]);
        return;
    }
    strlcpy(out, APP_SETUP_AP_SSID_PREFIX, out_len);
}

static bool wifi_mgr_normalize_country_code(const char *input, char *out, size_t out_len)
{
    if (input == NULL || out == NULL || out_len < APP_WIFI_COUNTRY_CODE_MAX_LEN) {
        return false;
    }
    if (strlen(input) != 2) {
        return false;
    }
    if (!isalpha((unsigned char)input[0]) || !isalpha((unsigned char)input[1])) {
        return false;
    }

    out[0] = (char)toupper((unsigned char)input[0]);
    out[1] = (char)toupper((unsigned char)input[1]);
    out[2] = '\0';
    return true;
}

static void wifi_mgr_set_country_code_from_input(const char *country_code)
{
    char normalized[APP_WIFI_COUNTRY_CODE_MAX_LEN] = {0};
    const char *source = (country_code != NULL && country_code[0] != '\0') ? country_code : APP_WIFI_COUNTRY_CODE;

    if (!wifi_mgr_normalize_country_code(source, normalized, sizeof(normalized))) {
        strlcpy(normalized, "US", sizeof(normalized));
    }
    strlcpy(s_wifi_country_code, normalized, sizeof(s_wifi_country_code));
}

static esp_err_t wifi_mgr_apply_country_code(void)
{
    if (s_wifi_country_code[0] == '\0') {
        wifi_mgr_set_country_code_from_input(NULL);
    }
    esp_err_t err = esp_wifi_set_country_code(s_wifi_country_code, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "esp_wifi_set_country_code(%s) failed: %s",
            s_wifi_country_code, esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG_WIFI, "Wi-Fi country code set to %s", s_wifi_country_code);
    return ESP_OK;
}

static const char *wifi_reason_to_str(uint8_t reason)
{
    switch (reason) {
    case 2:
        return "AUTH_EXPIRE";
    case 8:
        return "ASSOC_EXPIRE";
    case 15:
        return "4WAY_HANDSHAKE_TIMEOUT";
    case 39:
        return "BEACON_TIMEOUT";
    case 205:
        return "CONNECTION_FAIL";
    default:
        return "UNKNOWN";
    }
}

static void wifi_mgr_try_reconnect(uint8_t reason, int attempt_no);

static bool wifi_mgr_request_connect(bool force, const char *ctx)
{
    int64_t now_ms = esp_timer_get_time() / 1000;
    if (!force && (now_ms - s_last_connect_request_ms) < WIFI_CONNECT_MIN_GAP_MS) {
        ESP_LOGW(TAG_WIFI, "Skip esp_wifi_connect (%s): rate-limited (%" PRId64 " ms since last request)",
            (ctx != NULL) ? ctx : "n/a", (now_ms - s_last_connect_request_ms));
        return false;
    }

    s_last_connect_request_ms = now_ms;
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        ESP_LOGW(TAG_WIFI, "esp_wifi_connect failed (%s): %s",
            (ctx != NULL) ? ctx : "n/a", esp_err_to_name(err));
        return false;
    }
    return true;
}

static esp_err_t wifi_mgr_get_ip_for_netif(esp_netif_t *netif, char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    out[0] = '\0';
    if (netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_netif_ip_info_t ip_info = {0};
    esp_err_t err = esp_netif_get_ip_info(netif, &ip_info);
    if (err != ESP_OK) {
        return err;
    }
    if (ip_info.ip.addr == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    snprintf(out, out_len, IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}

static bool wifi_reason_is_reconnect_hard(uint8_t reason)
{
    switch (reason) {
    case 2:   /* AUTH_EXPIRE */
    case 8:   /* ASSOC_EXPIRE */
    case 15:  /* 4WAY_HANDSHAKE_TIMEOUT */
    case 205: /* CONNECTION_FAIL */
        return true;
    default:
        return false;
    }
}

static int64_t wifi_mgr_compute_reconnect_delay_ms(uint8_t reason, int attempt_no)
{
    bool hard_reason = wifi_reason_is_reconnect_hard(reason);
    int step = attempt_no;
    if (step < 1) {
        step = 1;
    } else if (step > 5) {
        step = 5;
    }

    int64_t delay_ms = hard_reason ? WIFI_RECONNECT_DELAY_HARD_MS : WIFI_RECONNECT_DELAY_MIN_MS;
    while (--step > 0) {
        delay_ms *= 2;
        if (delay_ms >= WIFI_RECONNECT_DELAY_MAX_MS) {
            delay_ms = WIFI_RECONNECT_DELAY_MAX_MS;
            break;
        }
    }
    return delay_ms;
}

static void wifi_mgr_reconnect_timer_cb(void *arg)
{
    (void)arg;
    if (s_wifi_connected) {
        return;
    }
    wifi_mgr_try_reconnect(s_pending_reconnect_reason, s_pending_reconnect_attempt);
}

static int64_t wifi_mgr_schedule_reconnect(uint8_t reason, int attempt_no)
{
    int64_t delay_ms = wifi_mgr_compute_reconnect_delay_ms(reason, attempt_no);
    s_pending_reconnect_reason = reason;
    s_pending_reconnect_attempt = attempt_no;

    if (s_reconnect_timer == NULL) {
        const esp_timer_create_args_t timer_args = {
            .callback = &wifi_mgr_reconnect_timer_cb,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "wifi_reconnect",
            .skip_unhandled_events = true,
        };
        esp_err_t create_err = esp_timer_create(&timer_args, &s_reconnect_timer);
        if (create_err != ESP_OK) {
            ESP_LOGW(TAG_WIFI, "Failed to create reconnect timer (%s), reconnecting immediately",
                esp_err_to_name(create_err));
            wifi_mgr_try_reconnect(reason, attempt_no);
            return 0;
        }
    }

    if (esp_timer_is_active(s_reconnect_timer)) {
        (void)esp_timer_stop(s_reconnect_timer);
    }

    esp_err_t start_err = esp_timer_start_once(s_reconnect_timer, (uint64_t)delay_ms * 1000ULL);
    if (start_err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "Failed to arm reconnect timer (%s), reconnecting immediately",
            esp_err_to_name(start_err));
        wifi_mgr_try_reconnect(reason, attempt_no);
        return 0;
    }

    return delay_ms;
}

static void wifi_mgr_try_reconnect(uint8_t reason, int attempt_no)
{
    int64_t now_ms = esp_timer_get_time() / 1000;
    bool hard_reason = wifi_reason_is_reconnect_hard(reason);
    bool reason_known = (reason != 0);

    if (hard_reason && attempt_no >= WIFI_RECOVER_HARD_ATTEMPT_THRESHOLD &&
        (now_ms - s_last_recover_stop_start_ms) >= WIFI_RECOVER_STOP_START_COOLDOWN_MS) {
        esp_err_t stop_err = esp_wifi_stop();
        if (stop_err != ESP_OK && stop_err != ESP_ERR_WIFI_NOT_INIT) {
            ESP_LOGW(TAG_WIFI, "esp_wifi_stop failed in recovery step: %s", esp_err_to_name(stop_err));
        }
        esp_err_t start_err = esp_wifi_start();
        if (start_err == ESP_OK || start_err == ESP_ERR_INVALID_STATE) {
            (void)wifi_mgr_apply_country_code();
            s_last_recover_stop_start_ms = now_ms;
            ESP_LOGW(TAG_WIFI, "Wi-Fi recovery step: stop/start (attempt %d, reason=%d)", attempt_no, (int)reason);
            (void)wifi_mgr_request_connect(true, "after-stop-start");
            return;
        }
        ESP_LOGW(TAG_WIFI, "esp_wifi_start failed in recovery step: %s", esp_err_to_name(start_err));
    }

    if (reason_known && (attempt_no % WIFI_RECOVER_DISC_CONN_PERIOD) == 0 &&
        (now_ms - s_last_recover_disc_conn_ms) >= WIFI_RECOVER_DISCONNECT_CONNECT_COOLDOWN_MS) {
        s_last_recover_disc_conn_ms = now_ms;
        if (wifi_mgr_request_connect(true, "periodic-connect-nudge")) {
            ESP_LOGW(TAG_WIFI, "Wi-Fi recovery step: periodic connect nudge (attempt %d, reason=%d)",
                attempt_no, (int)reason);
        }
        return;
    }

    (void)wifi_mgr_request_connect(false, "normal-recovery");
}

#if CONFIG_ESP_HOSTED_ENABLED
#define HOSTED_TRANSPORT_UP_BIT BIT0
#define HOSTED_TRANSPORT_FAIL_BIT BIT1
#define HOSTED_TRANSPORT_WAIT_MS 8000
#define HOSTED_C6_OTA_CHUNK_SIZE 1500U

static EventGroupHandle_t s_hosted_event_group = NULL;
static esp_event_handler_instance_t s_hosted_event_instance = NULL;

#if APP_HAVE_HOSTED_C6_FW_IMAGE
extern const uint8_t _binary_hosted_c6_fw_bin_start[] asm("_binary_hosted_c6_fw_bin_start");
extern const uint8_t _binary_hosted_c6_fw_bin_end[] asm("_binary_hosted_c6_fw_bin_end");
#endif
#endif

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static int retry_count = 0;
    static bool fail_notified = false;
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        wifi_event_sta_scan_done_t *scan_done = (wifi_event_sta_scan_done_t *)event_data;
        s_wifi_scan_last_status = (scan_done != NULL) ? scan_done->status : 0;
        if (s_wifi_event_group != NULL) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);
        }
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (s_wifi_event_group != NULL) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_STA_STARTED_BIT);
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
        if (s_wifi_event_group != NULL) {
            xEventGroupClearBits(s_wifi_event_group, WIFI_STA_STARTED_BIT);
        }
    }

    if (s_setup_ap_active) {
        if (event_base == IP_EVENT) {
            return;
        }
        if (event_base == WIFI_EVENT &&
            (event_id == WIFI_EVENT_STA_START ||
             event_id == WIFI_EVENT_STA_STOP ||
             event_id == WIFI_EVENT_STA_DISCONNECTED)) {
            return;
        }
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        fail_notified = false;
        if (s_reconnect_timer != NULL && esp_timer_is_active(s_reconnect_timer)) {
            (void)esp_timer_stop(s_reconnect_timer);
        }
#if APP_WIFI_DISABLE_POWER_SAVE
        esp_err_t ps_err = esp_wifi_set_ps(WIFI_PS_NONE);
        if (ps_err != ESP_OK) {
            ESP_LOGW(TAG_WIFI, "esp_wifi_set_ps(WIFI_PS_NONE) at STA_START failed: %s", esp_err_to_name(ps_err));
        }
#endif
        (void)wifi_mgr_request_connect(true, "sta-start");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disc = (wifi_event_sta_disconnected_t *)event_data;
        uint8_t reason = (disc != NULL) ? (uint8_t)disc->reason : 0;
        s_wifi_connected = false;
        if (disc != NULL) {
            ESP_LOGW(TAG_WIFI, "Wi-Fi disconnected, reason=%d (%s)",
                (int)disc->reason, wifi_reason_to_str((uint8_t)disc->reason));
        }
        retry_count++;
        int64_t delay_ms = wifi_mgr_schedule_reconnect(reason, retry_count);

        if (retry_count < s_wifi_max_retries) {
            ESP_LOGW(TAG_WIFI, "Wi-Fi reconnect attempt %d/%d scheduled in %" PRId64 " ms (reason=%d)",
                retry_count, s_wifi_max_retries, delay_ms, (int)reason);
        } else {
            if (!fail_notified && s_wifi_event_group != NULL) {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                fail_notified = true;
            }
            if ((retry_count % 10) == 0) {
                ESP_LOGW(TAG_WIFI, "Wi-Fi still reconnecting (attempt %d, configured max %d)", retry_count, s_wifi_max_retries);
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        s_wifi_connected = false;
        if (s_wifi_event_group != NULL) {
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
        retry_count++;
        int64_t delay_ms = wifi_mgr_schedule_reconnect(0, retry_count);
        ESP_LOGW(TAG_WIFI, "Wi-Fi lost IP, reconnect attempt %d/%d scheduled in %" PRId64 " ms",
            retry_count, s_wifi_max_retries, delay_ms);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        retry_count = 0;
        fail_notified = false;
        s_wifi_connected = true;
        s_last_recover_disc_conn_ms = 0;
        s_last_recover_stop_start_ms = 0;
        wifi_mgr_reset_reconnect_state();
#if APP_WIFI_DISABLE_POWER_SAVE
        esp_err_t ps_err = esp_wifi_set_ps(WIFI_PS_NONE);
        if (ps_err != ESP_OK) {
            ESP_LOGW(TAG_WIFI, "esp_wifi_set_ps(WIFI_PS_NONE) at GOT_IP failed: %s", esp_err_to_name(ps_err));
        }
#endif
        if (s_wifi_event_group != NULL) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
        ESP_LOGI(TAG_WIFI, "Wi-Fi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

#if CONFIG_ESP_HOSTED_ENABLED
#if CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE
static void wifi_mgr_pulse_hosted_reset_gpio(void)
{
#if defined(CONFIG_ESP_HOSTED_SDIO_GPIO_RESET_SLAVE)
    const int reset_gpio = CONFIG_ESP_HOSTED_SDIO_GPIO_RESET_SLAVE;
    if (reset_gpio < 0) {
        return;
    }
    if ((uint32_t)reset_gpio >= GPIO_PIN_COUNT) {
        ESP_LOGW(TAG_WIFI, "Skip manual C6 reset pulse: invalid GPIO %d", reset_gpio);
        return;
    }

    gpio_num_t reset_pin = (gpio_num_t)reset_gpio;
    esp_err_t cfg_err = gpio_reset_pin(reset_pin);
    if (cfg_err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "gpio_reset_pin(%d) failed before C6 reset pulse: %s",
            reset_gpio, esp_err_to_name(cfg_err));
    }
    cfg_err = gpio_set_direction(reset_pin, GPIO_MODE_OUTPUT);
    if (cfg_err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "gpio_set_direction(%d) failed before C6 reset pulse: %s",
            reset_gpio, esp_err_to_name(cfg_err));
        return;
    }

#if CONFIG_ESP_HOSTED_SDIO_RESET_ACTIVE_LOW
    const int inactive_level = 1;
    const int active_level = 0;
#else
    const int inactive_level = 0;
    const int active_level = 1;
#endif

    (void)gpio_set_level(reset_pin, inactive_level);
    vTaskDelay(pdMS_TO_TICKS(2));
    (void)gpio_set_level(reset_pin, active_level);
    vTaskDelay(pdMS_TO_TICKS(12));
    (void)gpio_set_level(reset_pin, inactive_level);

    ESP_LOGW(TAG_WIFI, "Manual C6 reset pulse on GPIO[%d]", reset_gpio);
#endif
}
#endif

static uint32_t hosted_version_pack(uint32_t major, uint32_t minor, uint32_t patch)
{
    return ESP_HOSTED_VERSION_VAL((major & 0xFFU), (minor & 0xFFU), (patch & 0xFFU));
}

static bool hosted_parse_version_text(const char *text, uint32_t *major, uint32_t *minor, uint32_t *patch)
{
    if (text == NULL || text[0] == '\0' || major == NULL || minor == NULL || patch == NULL) {
        return false;
    }
    unsigned int m1 = 0;
    unsigned int m2 = 0;
    unsigned int m3 = 0;
    if (sscanf(text, "%u.%u.%u", &m1, &m2, &m3) != 3) {
        return false;
    }
    if (m1 > 255U || m2 > 255U || m3 > 255U) {
        return false;
    }
    *major = (uint32_t)m1;
    *minor = (uint32_t)m2;
    *patch = (uint32_t)m3;
    return true;
}

#if APP_HAVE_HOSTED_C6_FW_IMAGE
static esp_err_t hosted_get_embedded_c6_fw(const uint8_t **out_data, size_t *out_len, char *out_version, size_t out_version_len)
{
    if (out_data == NULL || out_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t *start = _binary_hosted_c6_fw_bin_start;
    const uint8_t *end = _binary_hosted_c6_fw_bin_end;
    if (start == NULL || end == NULL || end <= start) {
        return ESP_ERR_NOT_FOUND;
    }

    size_t len = (size_t)(end - start);
    if (len < (sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))) {
        return ESP_ERR_INVALID_SIZE;
    }

    const esp_image_header_t *image_header = (const esp_image_header_t *)start;
    if (image_header->magic != ESP_IMAGE_HEADER_MAGIC) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    if (image_header->chip_id != ESP_CHIP_ID_ESP32C6) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    size_t app_desc_offset = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
    const esp_app_desc_t *app_desc = (const esp_app_desc_t *)(start + app_desc_offset);
    if (app_desc->magic_word != ESP_APP_DESC_MAGIC_WORD) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (out_version != NULL && out_version_len > 0) {
        strlcpy(out_version, app_desc->version, out_version_len);
    }
    *out_data = start;
    *out_len = len;
    return ESP_OK;
}
#endif

static void wifi_mgr_maybe_auto_update_c6_fw(const esp_hosted_coprocessor_fwver_t *running_fw)
{
    if (running_fw == NULL || !APP_HOSTED_AUTO_UPDATE_C6_FW) {
        return;
    }

    uint32_t host_version = hosted_version_pack(
        ESP_HOSTED_VERSION_MAJOR_1, ESP_HOSTED_VERSION_MINOR_1, ESP_HOSTED_VERSION_PATCH_1);
    uint32_t running_version = hosted_version_pack(running_fw->major1, running_fw->minor1, running_fw->patch1);

    if (running_version == host_version) {
        return;
    }

    ESP_LOGW(TAG_WIFI,
        "C6 FW mismatch: running %u.%u.%u, host expects " ESP_HOSTED_VERSION_PRINTF_FMT,
        (unsigned int)running_fw->major1,
        (unsigned int)running_fw->minor1,
        (unsigned int)running_fw->patch1,
        ESP_HOSTED_VERSION_PRINTF_ARGS(host_version));

#if !APP_HAVE_HOSTED_C6_FW_IMAGE
    ESP_LOGW(TAG_WIFI, "No embedded C6 firmware image available; skipping automatic C6 update");
    return;
#else
    const uint8_t *fw_data = NULL;
    size_t fw_len = 0;
    char fw_version_text[32] = {0};
    esp_err_t fw_meta_err = hosted_get_embedded_c6_fw(&fw_data, &fw_len, fw_version_text, sizeof(fw_version_text));
    if (fw_meta_err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "Embedded C6 firmware image invalid/unavailable (%s), skip auto update",
            esp_err_to_name(fw_meta_err));
        return;
    }

    uint32_t bundled_major = 0;
    uint32_t bundled_minor = 0;
    uint32_t bundled_patch = 0;
    if (hosted_parse_version_text(fw_version_text, &bundled_major, &bundled_minor, &bundled_patch)) {
        uint32_t bundled_version = hosted_version_pack(bundled_major, bundled_minor, bundled_patch);
        ESP_LOGW(TAG_WIFI,
            "Bundled C6 firmware version: %u.%u.%u (%u bytes)",
            (unsigned int)bundled_major, (unsigned int)bundled_minor, (unsigned int)bundled_patch, (unsigned int)fw_len);

        if (bundled_version != host_version) {
            ESP_LOGW(TAG_WIFI,
                "Bundled C6 version (%u.%u.%u) does not match host stack version ("
                ESP_HOSTED_VERSION_PRINTF_FMT "). Skipping auto update for safety.",
                (unsigned int)bundled_major,
                (unsigned int)bundled_minor,
                (unsigned int)bundled_patch,
                ESP_HOSTED_VERSION_PRINTF_ARGS(host_version));
            return;
        }

        if (bundled_version == running_version) {
            ESP_LOGW(TAG_WIFI, "Running C6 version already equals bundled image; skipping auto update");
            return;
        }
    } else {
        ESP_LOGW(TAG_WIFI,
            "Could not parse bundled C6 version string '%s'; skipping auto update for safety",
            fw_version_text);
        return;
    }

    ESP_LOGW(TAG_WIFI, "Starting automatic C6 OTA update over ESP-Hosted transport");
    ui_boot_splash_set_title("C6 Firmware Update");
    ui_boot_splash_clear_status();
    ui_boot_splash_set_status("Updating Wi-Fi coprocessor");
    ui_boot_splash_set_status("Do not disconnect power");
    ui_boot_splash_set_status("Please wait...");
    ui_boot_splash_set_progress(0);

    esp_err_t err = esp_hosted_slave_ota_begin();
    if (err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "esp_hosted_slave_ota_begin failed: %s", esp_err_to_name(err));
        ui_boot_splash_set_status("C6 update failed to start");
        return;
    }

    size_t offset = 0;
    uint8_t last_progress = 0;
    while (offset < fw_len) {
        size_t chunk_len = (fw_len - offset > HOSTED_C6_OTA_CHUNK_SIZE) ? HOSTED_C6_OTA_CHUNK_SIZE : (fw_len - offset);
        err = esp_hosted_slave_ota_write((uint8_t *)(fw_data + offset), (uint32_t)chunk_len);
        if (err != ESP_OK) {
            ESP_LOGW(TAG_WIFI, "esp_hosted_slave_ota_write failed at offset %u: %s",
                (unsigned int)offset, esp_err_to_name(err));
            ui_boot_splash_set_status("C6 update write failed");
            (void)esp_hosted_slave_ota_end();
            return;
        }
        offset += chunk_len;
        uint8_t progress = (uint8_t)((offset * 100U) / fw_len);
        if (progress != last_progress) {
            last_progress = progress;
            ui_boot_splash_set_progress(progress);
        }
    }

    err = esp_hosted_slave_ota_end();
    if (err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "esp_hosted_slave_ota_end failed: %s", esp_err_to_name(err));
        ui_boot_splash_set_status("C6 update finalize failed");
        return;
    }

    bool activate_supported =
        (running_fw->major1 > 2U) || ((running_fw->major1 == 2U) && (running_fw->minor1 > 5U));
    if (activate_supported) {
        err = esp_hosted_slave_ota_activate();
        if (err != ESP_OK) {
            ESP_LOGW(TAG_WIFI, "esp_hosted_slave_ota_activate failed: %s", esp_err_to_name(err));
            ui_boot_splash_set_status("C6 update activate failed");
            return;
        }
    }

    ui_boot_splash_set_progress(100);
    ui_boot_splash_set_status("C6 update done, rebooting...");
    ESP_LOGW(TAG_WIFI, "C6 OTA update completed. Rebooting host to resynchronize transport");
    esp_restart();
#endif
}

static void hosted_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base != ESP_HOSTED_EVENT || s_hosted_event_group == NULL) {
        return;
    }

    if (event_id == ESP_HOSTED_EVENT_TRANSPORT_UP) {
        xEventGroupSetBits(s_hosted_event_group, HOSTED_TRANSPORT_UP_BIT);
    } else if (event_id == ESP_HOSTED_EVENT_TRANSPORT_DOWN || event_id == ESP_HOSTED_EVENT_TRANSPORT_FAILURE) {
        xEventGroupSetBits(s_hosted_event_group, HOSTED_TRANSPORT_FAIL_BIT);
    } else if (event_id == ESP_HOSTED_EVENT_CP_INIT) {
        esp_hosted_event_init_t *event = (esp_hosted_event_init_t *)event_data;
        if (event != NULL) {
            ESP_LOGI(TAG_WIFI, "ESP-Hosted coprocessor init event, reset reason=%d", (int)event->reason);
        }
    }
}

static esp_err_t wifi_mgr_init_hosted_transport(void)
{
    if (s_hosted_transport_ready) {
        return ESP_OK;
    }

    if (s_hosted_event_group == NULL) {
        s_hosted_event_group = xEventGroupCreate();
    }
    if (s_hosted_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (s_hosted_event_instance == NULL) {
        ESP_RETURN_ON_ERROR(
            esp_event_handler_instance_register(
                ESP_HOSTED_EVENT, ESP_EVENT_ANY_ID, &hosted_event_handler, NULL, &s_hosted_event_instance),
            TAG_WIFI, "register ESP_HOSTED_EVENT");
    }

    int ret = esp_hosted_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WIFI, "esp_hosted_init failed: %s", esp_err_to_name((esp_err_t)ret));
        return (esp_err_t)ret;
    }

    xEventGroupClearBits(s_hosted_event_group, HOSTED_TRANSPORT_UP_BIT | HOSTED_TRANSPORT_FAIL_BIT);
    ret = esp_hosted_connect_to_slave();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WIFI, "esp_hosted_connect_to_slave failed: %s", esp_err_to_name((esp_err_t)ret));
        esp_hosted_deinit();
        return (esp_err_t)ret;
    }

    EventBits_t hosted_bits = xEventGroupWaitBits(
        s_hosted_event_group,
        HOSTED_TRANSPORT_UP_BIT | HOSTED_TRANSPORT_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(HOSTED_TRANSPORT_WAIT_MS));

    if (hosted_bits & HOSTED_TRANSPORT_FAIL_BIT) {
        ESP_LOGE(TAG_WIFI, "ESP-Hosted transport failure event while bringing link up");
        return ESP_FAIL;
    }
    if (!(hosted_bits & HOSTED_TRANSPORT_UP_BIT)) {
        ESP_LOGE(TAG_WIFI, "ESP-Hosted transport did not come up within %d ms", HOSTED_TRANSPORT_WAIT_MS);
        return ESP_ERR_TIMEOUT;
    }

    esp_hosted_coprocessor_fwver_t fw = {0};
    esp_err_t fw_err = esp_hosted_get_coprocessor_fwversion(&fw);
    if (fw_err != ESP_OK) {
        ESP_LOGW(TAG_WIFI,
                 "ESP-Hosted connected, but coprocessor FW version query failed (%s). Continuing anyway.",
                 esp_err_to_name(fw_err));
        // Not fatal: Waveshare example runs even without this response.
    } else {
        ESP_LOGI(TAG_WIFI, "ESP-Hosted connected to C6 FW %" PRIu32 ".%" PRIu32 ".%" PRIu32,
                 fw.major1, fw.minor1, fw.patch1);
        wifi_mgr_maybe_auto_update_c6_fw(&fw);
    }

    s_hosted_transport_ready = true;
    return ESP_OK;
}
#endif

static esp_err_t wifi_mgr_ensure_stack_initialized(void)
{
    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
    }
    if (s_wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

#if CONFIG_ESP_HOSTED_ENABLED
    ESP_RETURN_ON_ERROR(wifi_mgr_init_hosted_transport(), TAG_WIFI, "init ESP-Hosted transport");
#endif

    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wifi_init_cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG_WIFI, "esp_wifi_init failed: %s", esp_err_to_name(err));
        return err;
    }

    if (s_wifi_event_instance == NULL) {
        ESP_RETURN_ON_ERROR(
            esp_event_handler_instance_register(
                WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &s_wifi_event_instance),
            TAG_WIFI, "register WIFI_EVENT");
    }
    if (s_ip_event_instance == NULL) {
        ESP_RETURN_ON_ERROR(
            esp_event_handler_instance_register(
                IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &s_ip_event_instance),
            TAG_WIFI, "register IP_EVENT");
    }

    return ESP_OK;
}

esp_err_t wifi_mgr_init(const wifi_mgr_config_t *cfg)
{
    if (cfg == NULL || cfg->ssid == NULL || cfg->ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (cfg->max_retries < 0) {
        return ESP_ERR_INVALID_ARG;
    }

#if !SOC_WIFI_SUPPORTED && !CONFIG_ESP_HOSTED_ENABLED
    ESP_LOGW(TAG_WIFI, "Wi-Fi is not supported on target %s", CONFIG_IDF_TARGET);
    return ESP_ERR_NOT_SUPPORTED;
#endif

    s_wifi_connected = false;
    s_wifi_max_retries = (cfg->max_retries > 0) ? cfg->max_retries : WIFI_MAX_RETRIES_DEFAULT;
    ESP_LOGI(TAG_WIFI,
        "Wi-Fi recovery policy: hard stop/start >=%d attempts, periodic connect nudge every %d attempts (reason!=0)",
        WIFI_RECOVER_HARD_ATTEMPT_THRESHOLD, WIFI_RECOVER_DISC_CONN_PERIOD);

    esp_err_t err = wifi_mgr_ensure_stack_initialized();
    if (err != ESP_OK) {
        return err;
    }
    wifi_mgr_set_country_code_from_input(cfg->country_code);
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    if (s_wifi_sta_netif == NULL) {
        s_wifi_sta_netif = esp_netif_create_default_wifi_sta();
        if (s_wifi_sta_netif == NULL) {
            return ESP_FAIL;
        }
    }

    wifi_config_t wifi_cfg = {0};
    strlcpy((char *)wifi_cfg.sta.ssid, cfg->ssid, sizeof(wifi_cfg.sta.ssid));
    strlcpy((char *)wifi_cfg.sta.password, cfg->password ? cfg->password : "", sizeof(wifi_cfg.sta.password));
    wifi_cfg.sta.threshold.authmode =
        (cfg->password != NULL && cfg->password[0] != '\0') ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wifi_cfg.sta.pmf_cfg.capable = true;
    wifi_cfg.sta.pmf_cfg.required = false;
    memcpy(&s_cached_sta_cfg, &wifi_cfg, sizeof(s_cached_sta_cfg));
    s_cached_sta_cfg_valid = true;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG_WIFI, "esp_wifi_set_mode");
    wifi_mgr_set_setup_ap_state(false, NULL);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg), TAG_WIFI, "esp_wifi_set_config");
    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG_WIFI, "esp_wifi_start failed: %s", esp_err_to_name(err));
        return err;
    }
    (void)wifi_mgr_apply_country_code();

#if APP_WIFI_DISABLE_POWER_SAVE
    err = esp_wifi_set_ps(WIFI_PS_NONE);
    if (err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "esp_wifi_set_ps(WIFI_PS_NONE) failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG_WIFI, "Wi-Fi power save disabled (WIFI_PS_NONE)");
    }
#endif

    if (cfg->wait_for_ip) {
        const TickType_t timeout = pdMS_TO_TICKS((cfg->connect_timeout_ms > 0) ? cfg->connect_timeout_ms : 15000);
        EventBits_t bits = xEventGroupWaitBits(
            s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, timeout);

        if (bits & WIFI_CONNECTED_BIT) {
            return ESP_OK;
        }
        if (bits & WIFI_FAIL_BIT) {
            return ESP_FAIL;
        }
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

bool wifi_mgr_is_connected(void)
{
    return s_wifi_connected;
}

#if CONFIG_ESP_HOSTED_ENABLED
static esp_err_t wifi_mgr_force_hosted_hard_recover(void)
{
    int64_t now_ms = esp_timer_get_time() / 1000;
    if ((now_ms - s_last_hosted_hard_recover_ms) < WIFI_HOSTED_HARD_RECOVER_COOLDOWN_MS) {
        ESP_LOGW(TAG_WIFI, "Skip C6 hard recover: cooldown active (%" PRId64 " ms since last), fallback to Wi-Fi reconnect",
            now_ms - s_last_hosted_hard_recover_ms);
        return wifi_mgr_force_reconnect_internal(false);
    }

    s_wifi_connected = false;
    s_last_connect_request_ms = 0;
    s_last_recover_disc_conn_ms = 0;
    s_last_recover_stop_start_ms = 0;
    wifi_mgr_reset_reconnect_state();
    if (s_wifi_event_group != NULL) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    }

    esp_err_t stop_err = esp_wifi_stop();
    if (stop_err != ESP_OK &&
        stop_err != ESP_ERR_WIFI_NOT_INIT &&
        stop_err != ESP_ERR_WIFI_NOT_STARTED &&
        stop_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG_WIFI, "esp_wifi_stop failed during hard recover: %s", esp_err_to_name(stop_err));
    }

    int deinit_ret = esp_hosted_deinit();
    if (deinit_ret != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "esp_hosted_deinit during hard recover returned: %s", esp_err_to_name((esp_err_t)deinit_ret));
    }
    s_hosted_transport_ready = false;

#if CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE
    wifi_mgr_pulse_hosted_reset_gpio();
#endif

    esp_err_t hosted_err = wifi_mgr_init_hosted_transport();
    if (hosted_err != ESP_OK) {
        ESP_LOGE(TAG_WIFI, "C6 hard recover failed: ESP-Hosted re-init failed (%s)", esp_err_to_name(hosted_err));
        return hosted_err;
    }

    wifi_mode_t restore_mode = s_setup_ap_active ? WIFI_MODE_APSTA : WIFI_MODE_STA;

    esp_err_t mode_err = esp_wifi_set_mode(restore_mode);
    if (mode_err != ESP_OK && mode_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG_WIFI, "esp_wifi_set_mode failed during hard recover: %s", esp_err_to_name(mode_err));
        return mode_err;
    }

    if (s_cached_sta_cfg_valid) {
        esp_err_t cfg_err = esp_wifi_set_config(WIFI_IF_STA, &s_cached_sta_cfg);
        if (cfg_err != ESP_OK) {
            ESP_LOGW(TAG_WIFI, "esp_wifi_set_config(STA) failed during hard recover: %s", esp_err_to_name(cfg_err));
        }
    }

    esp_err_t start_err = esp_wifi_start();
    if (start_err != ESP_OK && start_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG_WIFI, "esp_wifi_start failed during hard recover: %s", esp_err_to_name(start_err));
        return start_err;
    }
    (void)wifi_mgr_apply_country_code();

#if APP_WIFI_DISABLE_POWER_SAVE
    esp_err_t ps_err = esp_wifi_set_ps(WIFI_PS_NONE);
    if (ps_err != ESP_OK) {
        ESP_LOGW(TAG_WIFI, "esp_wifi_set_ps(WIFI_PS_NONE) failed during hard recover: %s", esp_err_to_name(ps_err));
    }
#endif

    if (!wifi_mgr_request_connect(true, "after-c6-hard-recover")) {
        return ESP_FAIL;
    }

    s_last_hosted_hard_recover_ms = now_ms;
    ESP_LOGW(TAG_WIFI, "C6 hard recover complete: ESP-Hosted reinitialized and Wi-Fi reconnect requested");
    return ESP_OK;
}
#endif

static esp_err_t wifi_mgr_force_reconnect_internal(bool allow_transport_escalation)
{
#if !SOC_WIFI_SUPPORTED && !CONFIG_ESP_HOSTED_ENABLED
    (void)allow_transport_escalation;
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (s_wifi_sta_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    s_wifi_connected = false;
    s_last_connect_request_ms = 0;
    wifi_mgr_reset_reconnect_state();
    if (s_wifi_event_group != NULL) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }

    if (!wifi_mgr_request_connect(true, "force-reconnect")) {
        int64_t now_ms = esp_timer_get_time() / 1000;
        if ((now_ms - s_last_recover_stop_start_ms) >= WIFI_RECOVER_STOP_START_COOLDOWN_MS) {
            esp_err_t stop_err = esp_wifi_stop();
            if (stop_err != ESP_OK && stop_err != ESP_ERR_WIFI_NOT_INIT &&
                stop_err != ESP_ERR_WIFI_NOT_STARTED && stop_err != ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG_WIFI, "esp_wifi_stop failed during forced reconnect fallback: %s",
                    esp_err_to_name(stop_err));
            }
            esp_err_t start_err = esp_wifi_start();
            if (start_err == ESP_OK || start_err == ESP_ERR_INVALID_STATE) {
                (void)wifi_mgr_apply_country_code();
                s_last_recover_stop_start_ms = now_ms;
                if (wifi_mgr_request_connect(true, "force-reconnect-after-stop-start")) {
                    ESP_LOGW(TAG_WIFI, "Forced Wi-Fi reconnect fallback: stop/start + connect");
                    return ESP_OK;
                }
            } else {
                ESP_LOGW(TAG_WIFI, "esp_wifi_start failed during forced reconnect fallback: %s",
                    esp_err_to_name(start_err));
            }
        }
#if CONFIG_ESP_HOSTED_ENABLED
        if (allow_transport_escalation) {
            ESP_LOGW(TAG_WIFI,
                "Escalating forced reconnect failure to C6 hard recover");
            return wifi_mgr_force_hosted_hard_recover();
        }
#endif
        return ESP_FAIL;
    }

    ESP_LOGW(TAG_WIFI, "Forced Wi-Fi reconnect triggered");
    return ESP_OK;
#endif
}

esp_err_t wifi_mgr_force_reconnect(void)
{
    return wifi_mgr_force_reconnect_internal(true);
}

esp_err_t wifi_mgr_force_transport_recover(void)
{
#if !SOC_WIFI_SUPPORTED && !CONFIG_ESP_HOSTED_ENABLED
    return ESP_ERR_NOT_SUPPORTED;
#elif CONFIG_ESP_HOSTED_ENABLED
    return wifi_mgr_force_hosted_hard_recover();
#else
    return wifi_mgr_force_reconnect();
#endif
}

esp_err_t wifi_mgr_start_setup_ap(const wifi_mgr_ap_config_t *cfg)
{
#if !SOC_WIFI_SUPPORTED && !CONFIG_ESP_HOSTED_ENABLED
    return ESP_ERR_NOT_SUPPORTED;
#else
    esp_err_t err = wifi_mgr_ensure_stack_initialized();
    if (err != ESP_OK) {
        return err;
    }
    wifi_mgr_set_country_code_from_input((cfg != NULL) ? cfg->country_code : NULL);

    if (s_wifi_ap_netif == NULL) {
        s_wifi_ap_netif = esp_netif_create_default_wifi_ap();
        if (s_wifi_ap_netif == NULL) {
            return ESP_FAIL;
        }
    }

    char ap_ssid[APP_WIFI_SSID_MAX_LEN] = {0};
    if (cfg != NULL && cfg->ssid != NULL && cfg->ssid[0] != '\0') {
        strlcpy(ap_ssid, cfg->ssid, sizeof(ap_ssid));
    } else {
        wifi_mgr_build_default_setup_ssid(ap_ssid, sizeof(ap_ssid));
    }

    const char *ap_password =
        (cfg != NULL && cfg->password != NULL) ? cfg->password : APP_SETUP_AP_PASSWORD;
    uint8_t ap_channel =
        (cfg != NULL && cfg->channel > 0U) ? cfg->channel : (uint8_t)APP_SETUP_AP_CHANNEL;
    uint8_t ap_max_conn =
        (cfg != NULL && cfg->max_connection > 0U) ? cfg->max_connection : (uint8_t)APP_SETUP_AP_MAX_CONNECTIONS;

    wifi_config_t ap_cfg = {0};
    strlcpy((char *)ap_cfg.ap.ssid, ap_ssid, sizeof(ap_cfg.ap.ssid));
    ap_cfg.ap.ssid_len = (uint8_t)strlen(ap_ssid);
    ap_cfg.ap.channel = ap_channel;
    ap_cfg.ap.max_connection = ap_max_conn;
    ap_cfg.ap.pmf_cfg.capable = true;
    ap_cfg.ap.pmf_cfg.required = false;

    if (ap_password != NULL && ap_password[0] != '\0') {
        strlcpy((char *)ap_cfg.ap.password, ap_password, sizeof(ap_cfg.ap.password));
        ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        ap_cfg.ap.password[0] = '\0';
        ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
    }

    wifi_mgr_reset_reconnect_state();

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG_WIFI, "esp_wifi_set_mode(APSTA)");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg), TAG_WIFI, "esp_wifi_set_config(AP)");

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG_WIFI, "esp_wifi_start failed in AP setup mode: %s", esp_err_to_name(err));
        return err;
    }
    (void)wifi_mgr_apply_country_code();

    s_wifi_connected = false;
    if (s_wifi_event_group != NULL) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    }
    wifi_mgr_set_setup_ap_state(true, ap_ssid);
    ESP_LOGW(TAG_WIFI, "Setup AP active: SSID=%s", s_setup_ap_ssid);
    return ESP_OK;
#endif
}

esp_err_t wifi_mgr_stop_setup_ap(void)
{
#if !SOC_WIFI_SUPPORTED && !CONFIG_ESP_HOSTED_ENABLED
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!s_setup_ap_active) {
        return ESP_OK;
    }

    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_err_t mode_err = esp_wifi_get_mode(&mode);
    if (mode_err == ESP_OK && (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA)) {
        esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            return err;
        }
    } else if (mode_err != ESP_OK && mode_err != ESP_ERR_WIFI_NOT_INIT) {
        return mode_err;
    }

    wifi_mgr_set_setup_ap_state(false, NULL);
    return ESP_OK;
#endif
}

bool wifi_mgr_is_setup_ap_active(void)
{
    return s_setup_ap_active;
}

const char *wifi_mgr_get_setup_ap_ssid(void)
{
    return s_setup_ap_ssid;
}

esp_err_t wifi_mgr_get_sta_ip(char *out, size_t out_len)
{
#if !SOC_WIFI_SUPPORTED && !CONFIG_ESP_HOSTED_ENABLED
    (void)out;
    (void)out_len;
    return ESP_ERR_NOT_SUPPORTED;
#else
    return wifi_mgr_get_ip_for_netif(s_wifi_sta_netif, out, out_len);
#endif
}

esp_err_t wifi_mgr_get_ap_ip(char *out, size_t out_len)
{
#if !SOC_WIFI_SUPPORTED && !CONFIG_ESP_HOSTED_ENABLED
    (void)out;
    (void)out_len;
    return ESP_ERR_NOT_SUPPORTED;
#else
    return wifi_mgr_get_ip_for_netif(s_wifi_ap_netif, out, out_len);
#endif
}

esp_err_t wifi_mgr_scan(wifi_mgr_scan_result_t *results, size_t max_results, size_t *out_count)
{
#if !SOC_WIFI_SUPPORTED && !CONFIG_ESP_HOSTED_ENABLED
    (void)results;
    (void)max_results;
    if (out_count != NULL) {
        *out_count = 0;
    }
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (out_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_count = 0;
    if (results == NULL || max_results == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = wifi_mgr_ensure_stack_initialized();
    if (err != ESP_OK) {
        return err;
    }
    if (s_wifi_scan_in_progress) {
        return ESP_ERR_INVALID_STATE;
    }

    wifi_mode_t original_mode = WIFI_MODE_NULL;
    err = esp_wifi_get_mode(&original_mode);
    if (err != ESP_OK) {
        return err;
    }

    s_wifi_scan_in_progress = true;

    bool restore_mode = false;
    wifi_mode_t scan_mode = original_mode;
    if (original_mode == WIFI_MODE_AP) {
        scan_mode = WIFI_MODE_APSTA;
        restore_mode = true;
    } else if (original_mode == WIFI_MODE_NULL) {
        scan_mode = WIFI_MODE_STA;
        restore_mode = true;
    }
    bool wait_for_sta_start = (original_mode == WIFI_MODE_AP);
    EventBits_t clear_bits = WIFI_SCAN_DONE_BIT;
    if (wait_for_sta_start) {
        clear_bits |= WIFI_STA_STARTED_BIT;
    }

    if (s_wifi_event_group != NULL) {
        xEventGroupClearBits(s_wifi_event_group, clear_bits);
    }
    s_wifi_scan_last_status = 0;

    bool scan_started = false;
    if (restore_mode) {
        err = esp_wifi_set_mode(scan_mode);
        if (err != ESP_OK) {
            goto cleanup;
        }
    }

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        goto cleanup;
    }
    (void)wifi_mgr_apply_country_code();
    err = ESP_OK;

    if (wait_for_sta_start) {
        if (s_wifi_event_group == NULL) {
            err = ESP_ERR_INVALID_STATE;
            goto cleanup;
        }
        EventBits_t sta_bits = xEventGroupWaitBits(
            s_wifi_event_group,
            WIFI_STA_STARTED_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(WIFI_STA_START_WAIT_MS));
        if ((sta_bits & WIFI_STA_STARTED_BIT) == 0) {
            ESP_LOGW(TAG_WIFI, "Wi-Fi scan aborted: STA did not start within %d ms", WIFI_STA_START_WAIT_MS);
            err = ESP_ERR_TIMEOUT;
            goto cleanup;
        }
    }

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };

    err = esp_wifi_scan_start(&scan_cfg, false);
    if (err != ESP_OK) {
        goto cleanup;
    }
    scan_started = true;

    if (s_wifi_event_group == NULL) {
        err = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_SCAN_DONE_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(WIFI_SCAN_TIMEOUT_MS));
    if ((bits & WIFI_SCAN_DONE_BIT) == 0) {
        ESP_LOGW(TAG_WIFI, "Wi-Fi scan timed out after %d ms", WIFI_SCAN_TIMEOUT_MS);
        err = ESP_ERR_TIMEOUT;
        goto cleanup;
    }

    if (s_wifi_scan_last_status != 0) {
        ESP_LOGW(TAG_WIFI, "Wi-Fi scan failed (status=%d)", s_wifi_scan_last_status);
        err = ESP_FAIL;
        goto cleanup;
    }

    uint16_t found = 0;
    err = esp_wifi_scan_get_ap_num(&found);
    if (err != ESP_OK) {
        goto cleanup;
    }

    if (found > 0) {
        wifi_ap_record_t *records = calloc(found, sizeof(wifi_ap_record_t));
        if (records == NULL) {
            err = ESP_ERR_NO_MEM;
            goto cleanup;
        }

        uint16_t fetch_count = found;
        err = esp_wifi_scan_get_ap_records(&fetch_count, records);
        if (err == ESP_OK) {
            size_t out_idx = 0;
            for (uint16_t i = 0; i < fetch_count && out_idx < max_results; i++) {
                if (records[i].ssid[0] == '\0') {
                    continue;
                }
                bool duplicate = false;
                for (size_t j = 0; j < out_idx; j++) {
                    if (strncmp(results[j].ssid, (const char *)records[i].ssid, sizeof(results[j].ssid)) == 0) {
                        duplicate = true;
                        break;
                    }
                }
                if (duplicate) {
                    continue;
                }
                strlcpy(results[out_idx].ssid, (const char *)records[i].ssid, sizeof(results[out_idx].ssid));
                results[out_idx].rssi = records[i].rssi;
                results[out_idx].authmode = (uint8_t)records[i].authmode;
                out_idx++;
            }
            *out_count = out_idx;
        }
        free(records);
    }

cleanup:
    if (scan_started && err != ESP_OK) {
        (void)esp_wifi_scan_stop();
    }

    if (restore_mode) {
        esp_err_t restore_err = esp_wifi_set_mode(original_mode);
        if (err == ESP_OK && restore_err != ESP_OK) {
            err = restore_err;
        }
    }

    if (s_wifi_event_group != NULL) {
        xEventGroupClearBits(s_wifi_event_group, clear_bits);
    }
    s_wifi_scan_in_progress = false;
    return err;
#endif
}
