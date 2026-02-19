/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buf;
    size_t capacity;
    size_t read_idx;
    size_t write_idx;
    size_t len;
} ringbuf_t;

bool ringbuf_write_byte(ringbuf_t *rb, uint8_t value)
{
    if (rb == NULL || rb->buf == NULL || rb->len == rb->capacity) {
        return false;
    }
    rb->buf[rb->write_idx] = value;
    rb->write_idx = (rb->write_idx + 1U) % rb->capacity;
    rb->len++;
    return true;
}

bool ringbuf_read_byte(ringbuf_t *rb, uint8_t *out_value)
{
    if (rb == NULL || rb->buf == NULL || out_value == NULL || rb->len == 0U) {
        return false;
    }
    *out_value = rb->buf[rb->read_idx];
    rb->read_idx = (rb->read_idx + 1U) % rb->capacity;
    rb->len--;
    return true;
}
