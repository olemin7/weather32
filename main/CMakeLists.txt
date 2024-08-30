idf_component_register(SRCS 
                        "app_main.cpp" "provision.c" "mqtt_wrapper.cpp" "blink.cpp" 
                        "collector.cpp" "deepsleep.cpp" "utils.cpp" "bme280_wrapper.cpp"
                        INCLUDE_DIRS "." 
                    REQUIRES i2c_bus bme280 nvs_flash wifi_provisioning json esp_wifi mqtt
                    )
