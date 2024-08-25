idf_component_register(SRCS 
                        "app_main.cpp" "provision.c" "mqtt.cpp" "blink.cpp" 
                        "sensors.cpp"
                        INCLUDE_DIRS "." 
                    REQUIRES i2c_bus unity bme280 nvs_flash wifi_provisioning
                    )
