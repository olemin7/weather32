/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#include "kvs.hpp"
#include <esp_log.h>
#include <esp_system.h>

namespace kvs
{
    static const char *TAG = "KVS";

    handler::handler(const std::string &ns_name)
    {
        ESP_LOGI(TAG, "Init, ns=%s", ns_name.c_str());
        esp_err_t err;

        handle_ = nvs::open_nvs_handle(ns_name.c_str(), NVS_READWRITE, &err);
        ESP_ERROR_CHECK(err);
    }

    handler::~handler()
    {
        if (updated_)
        {
            handle_->commit();
        }
    }

    template <>
    esp_err_t handler::set_value(const std::string &key, const std::string value)
    {
        if (!handle_)
        {
            return ESP_ERR_INVALID_STATE;
        }
        return handle_->set_string(key.c_str(), value.c_str());
    }

    void init()
    {
        /* Initialize NVS partition */
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            /* NVS partition was truncated
             * and needs to be erased */
            ESP_ERROR_CHECK(nvs_flash_erase());

            /* Retry nvs_flash_init */
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
    }

} // namespace utils
