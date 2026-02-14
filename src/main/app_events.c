#include "app_events.h"

#include "esp_log.h"

#include "util/log_tags.h"

static QueueHandle_t s_event_queue = NULL;

esp_err_t app_events_init(void)
{
    if (s_event_queue != NULL) {
        return ESP_OK;
    }
    s_event_queue = xQueueCreate(APP_EVENT_QUEUE_LENGTH, sizeof(app_event_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG_APP, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

QueueHandle_t app_events_get_queue(void)
{
    return s_event_queue;
}

bool app_events_publish(const app_event_t *event, TickType_t timeout_ticks)
{
    if (s_event_queue == NULL || event == NULL) {
        return false;
    }
    return xQueueSend(s_event_queue, event, timeout_ticks) == pdTRUE;
}

bool app_events_receive(app_event_t *event, TickType_t timeout_ticks)
{
    if (s_event_queue == NULL || event == NULL) {
        return false;
    }
    return xQueueReceive(s_event_queue, event, timeout_ticks) == pdTRUE;
}
