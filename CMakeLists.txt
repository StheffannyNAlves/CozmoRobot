cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "/home/stheffanny/pico/pico-sdk")
endif()
set(PICO_BOARD pico_w)
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(cozmo_eyes C CXX ASM)

pico_sdk_init()

add_executable(cozmo_eyes
    main.c
    ssd1306.c
)

target_link_libraries(cozmo_eyes
    pico_stdlib
    hardware_i2c
)

pico_enable_stdio_usb(cozmo_eyes 0)
pico_enable_stdio_uart(cozmo_eyes 1)

pico_add_extra_outputs(cozmo_eyes)