#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t api_routes_register(httpd_handle_t server);

esp_err_t api_layout_get_handler(httpd_req_t *req);
esp_err_t api_layout_put_handler(httpd_req_t *req);
esp_err_t api_entities_get_handler(httpd_req_t *req);
esp_err_t api_state_get_handler(httpd_req_t *req);
esp_err_t api_settings_get_handler(httpd_req_t *req);
esp_err_t api_settings_put_handler(httpd_req_t *req);
esp_err_t api_wifi_scan_get_handler(httpd_req_t *req);
