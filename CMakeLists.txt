cmake_minimum_required(VERSION 3.16)
project(cozmo LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_executable(cozmo main.c eyes.c ssd1306.c botao.c)
target_compile_options(cozmo PRIVATE -Wall -Wextra -Wswitch-enum)