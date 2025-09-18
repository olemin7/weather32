/*
 *
 *  Created on: Jun 14, 2025
 *      Author: oleksandr
 */

#pragma once

#include <string>
#include "cJSON.h"
#include <optional>

namespace json_wrapper
{
    class read
    {
    protected:
        cJSON *json_ptr_;
        cJSON *get_field_(const std::string &name) const;

    public:
        read(cJSON *json_ptr);
        operator bool() const
        {
            return json_ptr_ != nullptr;
        }

        read get_field(const std::string &name) const;
        std::optional<std::string> get_field_as_string(const std::string &name) const; // any field return as string
        std::optional<std::string> get_field_string(const std::string &name) const;
        std::optional<double> get_field_number(const std::string &name) const;
        std::optional<bool> get_field_bool(const std::string &name) const;
        //--------- array
        std::optional<int> get_array_size() const; // empty - not array
        read get_array_item(int index) const;
    };

    class read_root : public read
    {
    private:
    public:
        read_root(const std::string &json_str);
        ~read_root();
    };

} // namespace utils
