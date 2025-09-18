/*
 *
 *  Created on: Jun 14, 2025
 *      Author: oleksandr
 */

#include "json_wrapper.hpp"
#include <esp_log.h>

namespace json_wrapper
{
    static const char *TAG = "proto_json_wrapper";

    read::read(cJSON *json_ptr) : json_ptr_(json_ptr)
    {
    }

    cJSON *read::get_field_(const std::string &name) const
    {
        if (json_ptr_ != nullptr)
        {
            return cJSON_GetObjectItem(json_ptr_, name.c_str());
        }
        else
        {
            return nullptr;
        }
    }

    read read::get_field(const std::string &name) const
    {
        return read(get_field_(name));
    }

    std::optional<std::string> read::get_field_as_string(const std::string &name) const
    {
        const auto field = get_field_(name);
        ESP_LOGD(TAG, "field %d name %s", field != nullptr, name.c_str());
        if (field)
        {
            auto c_str = cJSON_PrintUnformatted(field);
            std::string res = c_str;
            cJSON_free(c_str);
            return {res};
        }
        return {};
    }

    std::optional<std::string> read::get_field_string(const std::string &name) const
    {
        const auto field = get_field_(name);
        if (!field || !cJSON_IsString(field))
        {
            return {};
        }
        return {field->valuestring};
    }

    std::optional<double> read::get_field_number(const std::string &name) const
    {
        const auto field = get_field_(name);
        if (!field || !cJSON_IsNumber(field))
        {
            return {};
        }
        return field->valuedouble;
    }

    std::optional<bool> read::get_field_bool(const std::string &name) const
    {
        const auto field = get_field_(name);
        if (!field || !cJSON_IsBool(field))
        {
            return {};
        }
        return cJSON_IsTrue(field);
    }

    std::optional<int> read::get_array_size() const
    {
        if (json_ptr_ && cJSON_IsArray(json_ptr_))
        {
            return cJSON_GetArraySize(json_ptr_);
        }
        return {};
    }

    read read::get_array_item(int index) const
    {
        if (json_ptr_ && cJSON_IsArray(json_ptr_))
        {
            return read(cJSON_GetArrayItem(json_ptr_, index));
        }
        return read(nullptr);
    }

    read_root::read_root(const std::string &json_str) : read(cJSON_Parse(json_str.c_str()))
    {
    }
    read_root::~read_root()
    {
        if (json_ptr_)
        {
            cJSON_Delete(json_ptr_);
        }
    }

} // namespace json_wrapper
