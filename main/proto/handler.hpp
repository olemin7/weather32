/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <string>
#include "cJSON.h"
#include <map>
#include <functional>
#include <optional>

namespace proto
{
    using payload_t = std::optional<std::string>;

    class handler
    {
    public:
        using command_t = std::function<payload_t(const payload_t &&payload)>;

    private:
        struct cmd_info_t
        {
            command_t handler;
            std::string description;
        };
        std::map<std::string, cmd_info_t> handler_;

    public:
        handler();
        std::string on_command(const std::string &msg);
        void add(const std::string cmd, command_t &&handler, std::string &&description = "");
        std::string get_cmd_list() const;
    };

} // namespace utils
