# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)


set(EXTRA_COMPONENT_DIRS 
    ${EXTRA_COMPONENT_DIRS}
    $ENV{HOME}/personal/esp-idf/esp-idf-lib/components/si7021
    $ENV{HOME}/personal/esp-idf/esp-idf-lib/components/i2cdev
    $ENV{HOME}/personal/esp-idf/esp-idf-lib/components/esp_idf_lib_helpers
    $ENV{HOME}/personal/esp-idf/esp-idf-lib/components/max7219
    )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(clock32)
