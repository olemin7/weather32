#include "layers.hpp"
#include <algorithm>
#include "transformation.hpp"
#include "font.hpp"

namespace layers
{
    class data_str : public idata
    {
    private:
        const std::string str_;
        const screen::justify_t justify_;
        const uint8_t offset_;

    public:
        data_str(const std::string str, const screen::justify_t justify, const uint8_t offset) : str_(str), justify_(justify), offset_(offset) {}
        screen::buffer_t get() final
        {
            const auto image = font::get(str_);
            return transformation::image2buff(image, justify_, offset_);
        }
    };

    class data_buffer : public idata
    {
    private:
        std::function<screen::buffer_t()> &&buff_;

    public:
        data_buffer(std::function<screen::buffer_t()> &&buff) : buff_(std::move(buff)) {}
        screen::buffer_t get() final
        {
            return buff_();
        }
    };
    layers::layers() {}

    std::optional<uint8_t> layers::get_max_priority() const
    {
        if (layers_.empty())
        {
            return {};
        }
        uint8_t max = 0;
        for (const auto &el : layers_)
        {
            if (max < el.first)
            {
                max = el.first;
            }
        }
        return max;
    }

    void layers::draw(uint8_t priority)
    {
        screen::print(layers_[priority]->get());
    }

    void layers::show(uint8_t priority, std::unique_ptr<idata> &&obj)
    {

        const auto need_draw = (get_max_priority().value_or(0) <= priority);

        layers_[priority] = std::move(obj);
        if (need_draw)
        {
            draw(priority);
        }
    }

    void layers::cancel(uint8_t priority)
    {
        const auto need_draw = (get_max_priority() == priority);
        layers_.erase(priority);
        if (need_draw)
        {
            auto priority = get_max_priority();
            if (priority)
            {
                draw(priority.value());
            }
        }
    }

    void layers::show(uint8_t priority, std::function<screen::buffer_t()> &&buff)
    {

        show(priority, std::make_unique<data_buffer>(std::move(buff)));
    }

    void layers::show(uint8_t priority, const std::string str, const screen::justify_t justify, const uint8_t offset)
    {
        show(priority, std::make_unique<data_str>(str, justify, offset));
    }
}; // namespace blink