# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
#If IOT_SOLUTION_PATH is not defined, use relative path as default value
if(NOT DEFINED ENV{IOT_SOLUTION_PATH})
    error("IOT_SOLUTION_PATH not set")
endif()

set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS}
                        $ENV{IOT_SOLUTION_PATH}/components/i2c_bus
                        $ENV{IOT_SOLUTION_PATH}/components/sensors/humiture
                        $ENV{IOT_SOLUTION_PATH}/components/sensors/light_sensor
                        $ENV{IOT_SOLUTION_PATH}/components/sensors/pressure
                        )


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(weather32)
