cmake_minimum_required(VERSION 3.22)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(STM32Tools LANGUAGES C)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/cmake/Components.cmake")
# Firmware consumers select named components. Host tests remain an independent
# project at Test/ so target firmware toolchain flags do not leak into them.
