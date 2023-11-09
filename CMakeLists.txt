# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(WalkSensor C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(WalkSensor
        WalkSensor.c
        src/server_common.c
        src/mpu6050_i2c.c)

pico_set_program_name(WalkSensor "WalkSensor")
pico_set_program_version(WalkSensor "0.1")

pico_enable_stdio_uart(WalkSensor 1)
pico_enable_stdio_usb(WalkSensor 1)

# Add the standard library to the build
target_link_libraries(WalkSensor
        pico_stdlib
        pico_btstack_ble
        pico_btstack_cyw43
        pico_cyw43_arch_none
        hardware_adc
        hardware_i2c)

# Add the standard include files to the build (header files)
target_include_directories(WalkSensor PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include)

# Add GATT
pico_btstack_make_gatt_header(WalkSensor PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/temp_sensor.gatt")

pico_add_extra_outputs(WalkSensor)

