/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#include "handler.hpp"
#include "json_wrapper.hpp"
#include <esp_log.h>
#include <sstream>

namespace proto
{
    static const char *TAG = "proto_cmd_handler";

    handler::handler()
    {
        add("help", [this](auto)
            { return get_cmd_list(); });
    }

    std::string handler::on_command(const std::string &msg)
    {
        ESP_LOGI(TAG, "got msg =%s", msg.c_str());
        json_wrapper::read_root root(msg.c_str());
        payload_t response;
        if (!root)
        {
            ESP_LOGE(TAG, "no root");
            return "";
        }

        const auto cmd = root.get_field_string("cmd");

        if (cmd)
        {
            auto it = handler_.find(cmd.value());
            if (it != handler_.end())
            {
                auto payload = root.get_field_as_string("payload");
                if (!payload)
                {
                    ESP_LOGI(TAG, "!payload");
                }
                response = it->second.handler(std::move(payload));
            }
            else
            {
                ESP_LOGE(TAG, "no handler for cmd");
            }
        }
        else
        {
            ESP_LOGE(TAG, "!cmd");
        }
        return response.value_or("");
    }

    void handler::add(const std::string cmd, command_t &&handler, std::string &&description)
    {
        handler_[cmd] = cmd_info_t{handler, description};
    }

    std::string handler::get_cmd_list() const
    {
        std::stringstream ss;
        bool second = false;
        ss << "{";
        for (const auto &el : handler_)
        {
            if (second)
            {
                ss << ",";
                ss << std::endl;
            }
            else
            {
                second = true;
            }
            ss << R"({"cmd":")" << el.first;
            ss << R"(", "description":")" << el.second.description << R"("})";
        }
        ss << "}";
        return ss.str();
    }
}
