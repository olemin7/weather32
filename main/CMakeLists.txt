idf_component_register(
    SRCS
        "app_main.cpp" 
        "${my_idf_libs_SOURCE_DIR}/provision/provision.cpp"
        "${my_idf_libs_SOURCE_DIR}/display/blink.cpp" 
        "${my_idf_libs_SOURCE_DIR}/deepsleep.cpp"
        "${my_idf_libs_SOURCE_DIR}/sensors/bme680.cpp"
        "${my_idf_libs_SOURCE_DIR}/sensors/bh1750.cpp"
        "${my_idf_libs_SOURCE_DIR}/sensors/adc.cpp"
        "${my_idf_libs_SOURCE_DIR}/utils/utils.cpp" 
        "${my_idf_libs_SOURCE_DIR}/utils/kvs.cpp"
        #-----------------
        "${my_idf_libs_SOURCE_DIR}/test/tst_utils.cpp"
       
    INCLUDE_DIRS
        "."
        "${my_idf_libs_SOURCE_DIR}"
    REQUIRES
        nvs_flash 
        wifi_provisioning 
        json 
        esp_wifi 
        mqtt 
        driver 
        spi_flash
        unity
        esp_http_server
        i2cdev
        bme680
        bh1750
    )

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
