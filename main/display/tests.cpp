#include "tests.hpp"
#include "screen.hpp"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <array>
#include "font.hpp"
#include "transformation.hpp"
#include "sdkconfig.h"

namespace screen
{
    void test2()
    {
        auto image = font::get("123");
        for (int justify = 0; justify <= js_right; justify++)
        {
            print(transformation::image2buff(image, static_cast<justify_t>(justify), 0));
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    void test_center()
    {
        auto image = font::get("24:55");
        print(transformation::image2buff(image, js_center, 0));
        vTaskDelay(pdMS_TO_TICKS(2000));
        image = font::get("1:55");
        print(transformation::image2buff(image, js_center, 0));
        vTaskDelay(pdMS_TO_TICKS(2000));
        image = font::get("55");
        print(transformation::image2buff(image, js_center, 0));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    void tests()
    {
        test_center();
    }
}
