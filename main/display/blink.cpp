/*
 * blink.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: oleksandr
 */
#include "blink.hpp"
#include "esp_log.h"
#include "sdkconfig.h"
#include "led_indicator.h"

namespace blink {
    constexpr auto GPIO_LED_PIN = GPIO_NUM_8;
    constexpr auto GPIO_ACTIVE_LEVEL = true;
    static const char *TAG = "led_gpio";

    static led_indicator_handle_t led_handle = NULL;

    /**
     * @brief connecting to AP (or Cloud)
     *
     */
    static const blink_step_t default_connecting[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 200},
        {LED_BLINK_HOLD, LED_STATE_OFF, 800},
        {LED_BLINK_LOOP, 0, 0},
    };

    /**
     * @brief connected to AP (or Cloud) succeed
     *
     */
    static const blink_step_t default_connected[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 1000},
        {LED_BLINK_LOOP, 0, 0},
    };

    /**
     * @brief reconnecting to AP (or Cloud), if lose connection
     *
     */
    static const blink_step_t default_reconnecting[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 100},
        {LED_BLINK_HOLD, LED_STATE_OFF, 200},
        {LED_BLINK_LOOP, 0, 0},
    }; // offline

    /**
     * @brief updating software
     *
     */
    static const blink_step_t default_updating[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 50},
        {LED_BLINK_HOLD, LED_STATE_OFF, 100},
        {LED_BLINK_HOLD, LED_STATE_ON, 50},
        {LED_BLINK_HOLD, LED_STATE_OFF, 800},
        {LED_BLINK_LOOP, 0, 0},
    };

    /**
     * @brief restoring factory settings
     *
     */
    static const blink_step_t default_factory_reset[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 200},
        {LED_BLINK_HOLD, LED_STATE_OFF, 200},
        {LED_BLINK_LOOP, 0, 0},
    };

    /**
     * @brief provision done
     *
     */
    static const blink_step_t default_provisioned[] = {
        {LED_BLINK_HOLD, LED_STATE_OFF, 1000},
        {LED_BLINK_STOP, 0, 0},
    };

    /**
     * @brief provisioning
     *
     */
    static const blink_step_t default_provisioning[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 500},
        {LED_BLINK_HOLD, LED_STATE_OFF, 500},
        {LED_BLINK_LOOP, 0, 0},
    };

    /**
     * @brief LED indicator blink lists, the index like BLINK_FACTORY_RESET defined the priority of the blink
     *
     */
    blink_step_t const *led_indicator_blink_lists[] = {
        default_factory_reset,
        default_updating,
        default_connected,
        default_provisioned,
        default_reconnecting,
        default_connecting,
        default_provisioning,
        NULL,
    };

    void init()
    {
        if (led_handle != NULL)
        {
            ESP_LOGD(TAG, "inited");
            return;
        }
        ESP_LOGD(TAG, "init");
        led_indicator_gpio_config_t gpio_config = {
            .is_active_level_high = GPIO_ACTIVE_LEVEL,
            .gpio_num = GPIO_LED_PIN,
        };

        const led_indicator_config_t config = {
            .mode = LED_GPIO_MODE,
            .led_indicator_gpio_config = &gpio_config,
            .blink_lists = led_indicator_blink_lists,
            .blink_list_num = MAX_,
        };

        led_handle = led_indicator_create(&config);
        assert(led_handle != NULL);
    };

    void start(int state)
    {
        if (led_handle != NULL)
        {
            ESP_LOGI(TAG, "start blink: %d", static_cast<int>(state));
            led_indicator_start(led_handle, static_cast<int>(state));
        }
    }

    void stop(int state)
    {
        if (led_handle != NULL)
        {
            ESP_LOGI(TAG, "stop blink: %d", static_cast<int>(state));
            led_indicator_stop(led_handle, static_cast<int>(state));
        }
    }

} // namespace blink