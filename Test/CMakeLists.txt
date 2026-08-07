cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

project(32ModulesTest)
enable_language(C)

add_executable(${CMAKE_PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/refactor_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/ECSense.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/nor_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Protocol/modbus_codec.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Inc
)

add_executable(storage_test
    ${CMAKE_CURRENT_SOURCE_DIR}/storage_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/nor_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/storage_backend.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/storage_partition.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/storage_record.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/storage_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/storage_firmware.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/storage_upgrade.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/storage_commit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Flash/storage_bank.c
)

target_include_directories(storage_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Inc
)
