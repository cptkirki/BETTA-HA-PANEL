/* SPDX-License-Identifier: LicenseRef-FNCL-1.1
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

typedef esp_err_t (*http_guard_handler_t)(httpd_req_t *req);

esp_err_t http_guard_init(void);
esp_err_t http_guard_handle(httpd_req_t *req, http_guard_handler_t next_handler);
