#include "screen.hpp"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <max7219.h>
#include <array>
#include "esp_log.h"
#include "sensors/sensor_event.hpp"
#include "font.hpp"
#include "transformation.hpp"
#include "utils/kvs.hpp"
#include "utils/utils.hpp"

namespace screen
{
    constexpr auto TAG = "SCREEN";

    constexpr auto kvs_segment_rotation = "s_rotation";
    constexpr auto kvs_segment_upsidedown = "s_upsidedown";
    constexpr auto kvs_mirrored = "mirrored";
    constexpr auto kvs_brightness_point = "brigp";
    constexpr auto kvs_brightness_sz = "brigsz";

    max7219_t dev;

    uint8_t display_segment_rotation;
    bool display_segment_upsidedown;
    std::vector<brightness_point_t> brightness_;

    void ev_brightness(void * /*arg*/, esp_event_base_t /*event_base*/, int32_t /*event_id*/, void *event_data)
    {
        static auto pre_level = uint8_t{0};
        const auto update = (sensor_event::lighting_t *)event_data;
        ESP_LOGD(TAG, "lighting=%u", update->val);
        const auto brightness = utils::transform_ranges(brightness_, update->val);
        if (brightness != pre_level)
        {
            pre_level = brightness;
            max7219_set_brightness(&dev, pre_level);
            ESP_LOGD(TAG, "brightness=%u", pre_level);
        }
    }

    esp_err_t max7219_buffer_raw(const buffer_t &buffer)
    {
        uint8_t pos = 0;
        for (auto &line : buffer)
        {

            const auto ret = max7219_set_digit(&dev, pos, line);
            if (ret != ESP_OK)
            {
                return ret;
            }
            pos++;
        }
        return ESP_OK;
    }

    esp_err_t print(const buffer_t &buffer)
    {
        auto transformed = transformation::buffer_by_segment_rotate(buffer, display_segment_rotation);
        if (display_segment_upsidedown)
        {
            for (auto &line : transformed)
            {
                line = transformation::reverse_bits(line);
            }
        }
        return max7219_buffer_raw(transformed);
    }

    void test_rotation()
    {
        auto buffer = transformation::get_test_buffer();
        for (int i = 0; i < 4; i++)
        {

            max7219_buffer_raw(transformation::buffer_by_segment_rotate(buffer, i));
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    void startup_screen()
    {
        auto buffer = transformation::get_test_buffer();
        print(buffer);
    }

    void init()
    {
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
        bool display_mirrored;

        get_config(display_segment_rotation, display_segment_upsidedown, display_mirrored);
        brightness_ = get_config_brightness();

        ESP_LOGI(TAG, "segment_rotation=%d segment_upsidedown=%d mirrored=%d", display_segment_rotation, display_segment_upsidedown, display_mirrored);

        // Configure SPI bus
        spi_bus_config_t cfg = {
            .mosi_io_num = GPIO_NUM_6,
            .miso_io_num = -1,
            .sclk_io_num = GPIO_NUM_4,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 0,
            .flags = 0};
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &cfg, SPI_DMA_CH_AUTO));

        // Configure device
        dev = max7219_t{
            .digits = 0,
            .cascade_size = CONFIG_DISPLAY_SEGMENTS,
            .mirrored = display_mirrored,
        };
        ESP_ERROR_CHECK(max7219_init_desc(&dev, SPI2_HOST, MAX7219_MAX_CLOCK_SPEED_HZ / 2, GPIO_NUM_5));
        ESP_ERROR_CHECK(max7219_init(&dev));
        ESP_ERROR_CHECK(max7219_clear(&dev));

        ESP_ERROR_CHECK(esp_event_handler_register(sensor_event::event, sensor_event::lighting, &ev_brightness, NULL));
        //    test_rotation();
        startup_screen();
    }

    esp_err_t print(const std::vector<uint8_t> &image, const justify_t justify, const uint8_t offset)
    {
        const auto buffer = transformation::image2buff(image, justify, offset);
        return print(buffer);
    }

    esp_err_t print(const std::string str, const justify_t justify, const uint8_t offset)
    {
        const auto image = font::get(str);
        return print(image, justify, offset);
    }

    esp_err_t set_config(uint8_t segment_rotation,
                         bool segment_upsidedown,
                         bool mirrored)
    {
        auto kvss = kvs::handler(TAG);

        ESP_LOGI(TAG, "rotation %d, upsidedown %d, mirrored %d", segment_rotation, segment_upsidedown, mirrored);

        bool error = false;

        if (ESP_OK != kvss.set_value(kvs_segment_rotation, segment_rotation))
        {
            error = true;
        }

        if (ESP_OK != kvss.set_value(kvs_segment_upsidedown, segment_upsidedown))
        {
            error = true;
        }

        if (ESP_OK != kvss.set_value(kvs_mirrored, mirrored))
        {
            error = true;
        }

        if (error)
        {
            ESP_LOGE(TAG, "error wr");
        }

        return ESP_OK;
    }

    void get_config(uint8_t &segment_rotation,
                    bool &segment_upsidedown,
                    bool &mirrored)
    {
        auto kvss = kvs::handler(TAG);
        kvss.get_value_or(kvs_segment_rotation, segment_rotation, CONFIG_DISPLAY_SEGMENT_ROTATION);
        kvss.get_value_or(kvs_segment_upsidedown, segment_upsidedown, CONFIG_DISPLAY_SEGMENT_UPSIDEDOWN);
        kvss.get_value_or(kvs_mirrored, mirrored, CONFIG_DISPLAY_MIRRORED);
    }

    esp_err_t set_config_brightness(const std::vector<brightness_point_t> &points)
    {
        const auto sz = points.size();
        ESP_LOGI(TAG, "brightness points=%d", sz);
        if (sz == 0)
        {
            return ESP_OK;
        }
        esp_err_t err = ESP_OK;
        do
        {
            auto kvss = kvs::handler(TAG);
            err = kvss.set_value(kvs_brightness_sz, sz);
            if (ESP_OK != err)
            {
                break;
            }
            size_t index = 0;
            for (const auto &point : points)
            {
                const uint16_t packed = point.first << 4 | (point.second & 0xf);
                err = kvss.set_value(kvs_brightness_point + std::to_string(index), packed);
                if (ESP_OK != err)
                {
                    break;
                }
                index++;
            }
            if (ESP_OK == err)
            {
                return ESP_OK;
            }
        } while (0);

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "error %s", esp_err_to_name(err));
        }
        return ESP_FAIL;
    }

    std::vector<brightness_point_t> get_config_brightness()
    {
        esp_err_t err = ESP_OK;
        do
        {
            auto kvss = kvs::handler(TAG);
            size_t sz;
            err = kvss.get_value(kvs_brightness_sz, sz);
            if (ESP_OK != err || sz == 0)
            {
                break;
            }
            ESP_LOGI(TAG, "brightness points=%d", sz);
            std::vector<brightness_point_t> points;
            size_t index;
            for (index = 0; index < sz; index++)
            {
                uint16_t packed;
                err = kvss.get_value(kvs_brightness_point + std::to_string(index), packed);

                if (ESP_OK != err)
                {
                    break;
                }
                points.push_back({packed >> 4, packed & 0xf});
            }
            if (err == ESP_OK)
            {
                return points;
            }
        } while (0);
        // default
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "error %s", esp_err_to_name(err));
        }
        ESP_LOGD(TAG, "used default brightness");
        return {{200, 0}, {700, 15}};
    }
}
