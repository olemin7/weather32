/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <nvs_flash.h>
#include <nvs.h>
#include <nvs_handle.hpp>
#include <string>
#include <memory>

namespace kvs
{
    // kvs key_max = 15;
    class handler
    {

    private:
        std::unique_ptr<nvs::NVSHandle> handle_;
        bool updated_ = false;
        template <typename T>
        esp_err_t set_item_(const std::string &key, T value)
        {
            if (!handle_)
            {
                return ESP_ERR_INVALID_STATE;
            }

            const auto ret = handle_->set_item(key.c_str(), value);
            if (ESP_OK == ret)
            {
                updated_ = true;
            }

            return ret;
        }

        esp_err_t set_item_(const std::string &key, const std::string value)
        {
            if (!handle_)
            {
                return ESP_ERR_INVALID_STATE;
            }

            const auto ret = handle_->set_string(key.c_str(), value.c_str());
            if (ESP_OK == ret)
            {
                updated_ = true;
            }

            return ret;
        }

    public:
        handler(const std::string &ns_name);
        ~handler();

        // do not update value if error
        template <typename T>
        esp_err_t get_value(const std::string &key, T &value)
        {
            if (!handle_)
            {
                return ESP_ERR_INVALID_STATE;
            }
            return handle_->get_item(key.c_str(), value);
        }

        esp_err_t get_value(const std::string &key, std::string &value)
        {
            if (!handle_)
            {
                return ESP_ERR_INVALID_STATE;
            }
            //    virtual esp_err_t get_string(const char *key, char* out_str, size_t len) = 0;
            char tt[255];
            const auto res = handle_->get_string(key.c_str(), tt, sizeof(tt));
            if (ESP_OK == res)
            {
                value = tt;
            }
            return res;
        }

        template <typename T, typename D>
        void get_value_or(const std::string &key, T &value, const D def_val)
        {
            if (ESP_OK != get_value(key, value))
            {
                value = def_val;
            }
        }

        template <typename T>
        esp_err_t set_value(const std::string &key, T value)
        {
            if (!handle_)
            {
                return ESP_ERR_INVALID_STATE;
            }
            T saved_val;
            auto ret = ESP_OK;
            if (ESP_OK != get_value(key, saved_val) || saved_val != value)
            {
                ret = set_item_(key.c_str(), value);
                if (ESP_OK == ret)
                {
                    updated_ = true;
                }
            }

            return ret;
        }
    };

    void init();

} // namespace utils
