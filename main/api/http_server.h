/* SPDX-License-Identifier: LicenseRef-FNCL-1.0
 * Copyright (c) 2026 Christopher Gleiche
 */
#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t http_server_start(void);
void http_server_stop(void);
httpd_handle_t http_server_get_handle(void);
