#pragma once
#include <string>
#include <functional>
#include <map>
#include "esp_http_server.h"

namespace http_server
{
    class server
    {
    public:
        using uri_handler_t = std::function<std::string(const std::string &)>;

    private:
        std::map<std::string, uri_handler_t> uri_handlers_;
        httpd_handle_t httpd_ = nullptr;
        server();

        static void disconnect_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data);
        static void connect_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);

    public:
        static server &get_instance();

        server(const server &) = delete;
        server &operator=(const server &) = delete;

        void set_uri(const std::string &uri, uri_handler_t &&handler);
    };
}