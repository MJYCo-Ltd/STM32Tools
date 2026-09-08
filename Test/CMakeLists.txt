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
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Bootloader/bootloader_policy.c
)

target_include_directories(storage_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Inc
)

add_executable(ml307_http_test
    ${CMAKE_CURRENT_SOURCE_DIR}/ml307_http_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/ML307/ml307_http.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Common.c
)

target_include_directories(ml307_http_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Inc
)

# Reusable parsers and signature verification; no Agriculture/HAL dependency.
add_library(module_frame_parser STATIC ../Src/AT/ModuleFrameParser.c)
target_include_directories(module_frame_parser PUBLIC ../Inc)

add_library(signed_firmware_core STATIC
    ../Src/Flash/SignedFirmware.c
    ../ThirdParty/monocypher/monocypher.c
    ../ThirdParty/monocypher/monocypher-ed25519.c)
target_include_directories(signed_firmware_core PUBLIC ../Inc
    PRIVATE ../ThirdParty/monocypher)

enable_testing()
add_executable(mqtt_line_collector_test mqtt_line_collector_test.c)
target_include_directories(mqtt_line_collector_test PRIVATE ../Inc)
add_test(NAME mqtt_line_collector_test COMMAND mqtt_line_collector_test)

# Generic storage transaction tests use I/O doubles, without board dependencies.
add_executable(dual_bank_transaction_test
    dual_bank_transaction_test.c ../Src/Flash/DualBankStore.c)
target_include_directories(dual_bank_transaction_test PRIVATE ../Inc)
add_test(NAME dual_bank_transaction_test COMMAND dual_bank_transaction_test)
