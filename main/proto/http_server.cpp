/*
 * blink.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: oleksandr
 */
#include "http_server.hpp"

#include <inttypes.h>
#include <stdio.h>
#include <chrono>
#include "esp_log.h"
#include <esp_wifi.h>
#include <memory>

using namespace std::chrono_literals;
static const char *TAG = "http_server";

namespace http_server
{
    static esp_err_t cmd_handler(httpd_req_t *req)
    {
        // Limit payload size for safety
        const size_t max_len = 1024;
        int total_len = req->content_len;
        int received = 0;

        if (total_len >= max_len)
        {
            ESP_LOGW(TAG, "POST payload too large");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Payload too large");
            return ESP_FAIL;
        }

        std::string buf;
        buf.resize(total_len);

        while (received < total_len)
        {
            int ret = httpd_req_recv(req, &buf[received], total_len - received);
            if (ret <= 0)
            {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                {
                    httpd_resp_send_408(req);
                }
                return ESP_FAIL;
            }
            received += ret;
        }

        ESP_LOGI(TAG, "Received POST JSON: %s", buf.c_str());
        if (req->user_ctx == NULL)
        {
            ESP_LOGE(TAG, "No user context set for URI handler");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No user context set for URI handler");
            return ESP_FAIL;
        }
        auto responce = reinterpret_cast<server::uri_handler_t *>(req->user_ctx)->operator()(buf);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, responce.c_str(), responce.length()); // Respond with empty JSON object
        return ESP_OK;
    }

    void server::disconnect_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
    {
        ESP_LOGI(TAG, "Disconnect event received, stopping webserver");
        if (arg == NULL)
        {
            ESP_LOGE(TAG, "Disconnect handler called with NULL argument");
            return;
        }
        auto httpd = reinterpret_cast<server *>(arg)->httpd_;
        if (httpd != NULL)
        {
            // Stop the webserver
            if (httpd_stop(httpd) == ESP_OK)
            {
                httpd = NULL;
            }
            else
            {
                ESP_LOGE(TAG, "Failed to stop http server");
            }
        }
    }

    void server::connect_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)

    {
        ESP_LOGI(TAG, "Connect event received, starting webserver");
        if (arg == NULL)
        {
            ESP_LOGE(TAG, "Connect handler called with NULL argument");
            return;
        }
        auto instance = reinterpret_cast<server *>(arg);
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();

        // Empty handle to http_server
        instance->httpd_ = NULL;
        ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
        // Start the httpd server
        if (httpd_start(&instance->httpd_, &config) == ESP_OK)
        {
            // Register URI handlers
            httpd_uri_t uri_handler = {

                .method = HTTP_POST,
                .handler = cmd_handler,
                .user_ctx = NULL};
            for (const auto &pair : instance->uri_handlers_)
            {
                uri_handler.uri = pair.first.c_str();
                uri_handler.user_ctx = (void *)(&pair.second);
                ESP_LOGI(TAG, "Registering URI handler for: %s", uri_handler.uri);
                esp_err_t err = httpd_register_uri_handler(instance->httpd_, &uri_handler);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to register URI handler for: %s", uri_handler.uri);
                }
            }
        }
    }

    server::server()
    {
        ESP_LOGI(TAG, "http_server::server()");
        // Initialize the HTTP server
        // Register event handlers for connection and disconnection events
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, this));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, this));
    }

    server &server::get_instance()
    {
        static std::unique_ptr<server> instance_;
        if (!instance_)
        {
            instance_ = std::unique_ptr<server>(new server());
            ESP_LOGD(TAG, "http_server::server::get_instance() created");
        }
        return *instance_;
    }

    void server::set_uri(const std::string &uri, uri_handler_t &&handler)
    {
        ESP_LOGI(TAG, "http_server::server::set_uri() URI handler set for: %s", uri.c_str());
        uri_handlers_[uri] = std::move(handler);
    }
}