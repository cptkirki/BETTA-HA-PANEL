#include "api/http_guard.h"

#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#include "util/log_tags.h"

#define HTTP_GUARD_MAX_ACTIVE_REQUESTS 2
#define HTTP_GUARD_MAX_CLIENTS 16
#define HTTP_GUARD_RATE_PER_SEC 5
#define HTTP_GUARD_BURST 10

typedef struct {
    bool used;
    uint32_t key;
    int32_t tokens_milli;
    int64_t last_refill_ms;
    int64_t last_seen_ms;
} http_guard_bucket_t;

static SemaphoreHandle_t s_active_sem = NULL;
static SemaphoreHandle_t s_guard_lock = NULL;
static bool s_inited = false;
static http_guard_bucket_t s_buckets[HTTP_GUARD_MAX_CLIENTS];

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static uint32_t req_client_key(httpd_req_t *req)
{
    if (req == NULL) {
        return 0;
    }

    int sockfd = httpd_req_to_sockfd(req);
    if (sockfd < 0) {
        return 0;
    }

    struct sockaddr_storage addr = {0};
    socklen_t addr_len = sizeof(addr);
    if (getpeername(sockfd, (struct sockaddr *)&addr, &addr_len) != 0) {
        return 0;
    }

    if (addr.ss_family == AF_INET && addr_len >= sizeof(struct sockaddr_in)) {
        const struct sockaddr_in *sa = (const struct sockaddr_in *)&addr;
        return sa->sin_addr.s_addr;
    }

#if LWIP_IPV6
    if (addr.ss_family == AF_INET6 && addr_len >= sizeof(struct sockaddr_in6)) {
        const struct sockaddr_in6 *sa6 = (const struct sockaddr_in6 *)&addr;
        const uint8_t *a = (const uint8_t *)&sa6->sin6_addr;
        uint32_t h = 2166136261u;
        for (size_t i = 0; i < 16; i++) {
            h ^= a[i];
            h *= 16777619u;
        }
        return h;
    }
#endif

    return 0;
}

static bool rate_limit_allow(uint32_t client_key)
{
    const int32_t burst_milli = HTTP_GUARD_BURST * 1000;
    const int64_t t_now = now_ms();
    int idx = -1;
    int free_idx = -1;
    int oldest_idx = -1;
    int64_t oldest_seen = INT64_MAX;

    for (int i = 0; i < HTTP_GUARD_MAX_CLIENTS; i++) {
        if (s_buckets[i].used) {
            if (s_buckets[i].key == client_key) {
                idx = i;
                break;
            }
            if (s_buckets[i].last_seen_ms < oldest_seen) {
                oldest_seen = s_buckets[i].last_seen_ms;
                oldest_idx = i;
            }
        } else if (free_idx < 0) {
            free_idx = i;
        }
    }

    if (idx < 0) {
        idx = (free_idx >= 0) ? free_idx : oldest_idx;
        if (idx < 0) {
            return false;
        }
        s_buckets[idx].used = true;
        s_buckets[idx].key = client_key;
        s_buckets[idx].tokens_milli = burst_milli;
        s_buckets[idx].last_refill_ms = t_now;
        s_buckets[idx].last_seen_ms = t_now;
    }

    http_guard_bucket_t *b = &s_buckets[idx];
    int64_t elapsed_ms = t_now - b->last_refill_ms;
    if (elapsed_ms > 0) {
        int64_t refill_milli = elapsed_ms * HTTP_GUARD_RATE_PER_SEC;
        int64_t tokens = (int64_t)b->tokens_milli + refill_milli;
        if (tokens > burst_milli) {
            tokens = burst_milli;
        }
        b->tokens_milli = (int32_t)tokens;
        b->last_refill_ms = t_now;
    }
    b->last_seen_ms = t_now;

    if (b->tokens_milli < 1000) {
        return false;
    }

    b->tokens_milli -= 1000;
    return true;
}

static esp_err_t send_busy(httpd_req_t *req, const char *status, const char *message)
{
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Retry-After", "1");
    return httpd_resp_sendstr(req, message);
}

esp_err_t http_guard_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    if (s_guard_lock == NULL) {
        s_guard_lock = xSemaphoreCreateMutex();
    }
    if (s_guard_lock == NULL) {
        return ESP_ERR_NO_MEM;
    }

    xSemaphoreTake(s_guard_lock, portMAX_DELAY);
    if (!s_inited) {
        if (s_active_sem == NULL) {
            s_active_sem = xSemaphoreCreateCounting(HTTP_GUARD_MAX_ACTIVE_REQUESTS, HTTP_GUARD_MAX_ACTIVE_REQUESTS);
        }
        if (s_active_sem == NULL) {
            xSemaphoreGive(s_guard_lock);
            return ESP_ERR_NO_MEM;
        }
        for (int i = 0; i < HTTP_GUARD_MAX_CLIENTS; i++) {
            s_buckets[i].used = false;
        }
        s_inited = true;
    }
    xSemaphoreGive(s_guard_lock);
    return ESP_OK;
}

esp_err_t http_guard_handle(httpd_req_t *req, http_guard_handler_t next_handler)
{
    if (req == NULL || next_handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t init_err = http_guard_init();
    if (init_err != ESP_OK) {
        ESP_LOGW(TAG_HTTP, "HTTP guard init failed: %s", esp_err_to_name(init_err));
        return send_busy(req, "503 Service Unavailable", "Service unavailable");
    }

    uint32_t key = req_client_key(req);
    bool allow_rate = false;

    xSemaphoreTake(s_guard_lock, portMAX_DELAY);
    allow_rate = rate_limit_allow(key);
    xSemaphoreGive(s_guard_lock);

    if (!allow_rate) {
        return send_busy(req, "429 Too Many Requests", "Too many requests");
    }

    if (xSemaphoreTake(s_active_sem, 0) != pdTRUE) {
        return send_busy(req, "503 Service Unavailable", "Server busy");
    }

    esp_err_t err = next_handler(req);
    xSemaphoreGive(s_active_sem);
    return err;
}
