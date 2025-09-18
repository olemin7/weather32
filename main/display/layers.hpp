#pragma once
#include "screen.hpp"
#include <stdint.h>
#include <map>
#include <memory>
#include <functional>

namespace layers
{
    class idata
    {
    public:
        virtual screen::buffer_t get() = 0;
    };

    class layers
    {
    private:
        std::map<uint8_t, std::unique_ptr<idata>> layers_;
        std::optional<uint8_t> get_max_priority() const;
        void draw(uint8_t priority);

    public:
        layers();
        void show(uint8_t priority, std::unique_ptr<idata> &&obj);
        void show(uint8_t priority, std::function<screen::buffer_t()> &&buff);
        void show(uint8_t priority, const std::string str, const screen::justify_t justify = screen::js_left, const uint8_t offset = 0);
        void cancel(uint8_t priority);
    };

}; // namespace blink