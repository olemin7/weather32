# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include(FetchContent)
FetchContent_Declare(
  espidflib
  GIT_REPOSITORY https://github.com/UncleRus/esp-idf-lib.git
)

#to be able update libraries
if(DEFINED ENV{MY_ESP_LIBRARIES_LOCAL})
  FetchContent_Declare(
    my_idf_libs
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../esp32_idf_libs
  )
else()
  FetchContent_Declare(
    my_idf_libs
    GIT_REPOSITORY git@github.com:olemin7/esp32_idf_libs.git
    GIT_TAG main  # Replace with your desired tag
  )
endif()


FetchContent_MakeAvailable(espidflib my_idf_libs)

set(EXTRA_COMPONENT_DIRS 
    ${EXTRA_COMPONENT_DIRS}
    ${espidflib_SOURCE_DIR}/components/i2cdev
    ${espidflib_SOURCE_DIR}/components/bme680
    ${espidflib_SOURCE_DIR}/components/bh1750
    ${espidflib_SOURCE_DIR}/components/esp_idf_lib_helpers
    ${my_idf_libs_SOURCE_DIR}/mqtt_tools
    )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(weather32)
