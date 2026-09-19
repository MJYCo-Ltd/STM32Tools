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
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/HostTests.cmake")
add_test(NAME ${CMAKE_PROJECT_NAME} COMMAND ${CMAKE_PROJECT_NAME})
add_test(NAME storage_test COMMAND storage_test)
add_test(NAME ml307_http_test COMMAND ml307_http_test)

add_executable(at_channel_recovery_test at_channel_recovery_test.c)
target_include_directories(at_channel_recovery_test PRIVATE ../Inc)
add_test(NAME at_channel_recovery_test COMMAND at_channel_recovery_test)

add_executable(sensor_driver_test
    sensor_driver_test.c
    ../Src/AHT20/aht20.c
    ../Src/TMP/tmp117.c
    ../Src/Bus/i2c_bus.c
    ../Src/Common.c)
target_include_directories(sensor_driver_test PRIVATE ../Inc)
add_test(NAME sensor_driver_test COMMAND sensor_driver_test)

add_executable(st7305_driver_test
    st7305_driver_test.c
    ../Src/Display/LCD/lcd_st7305.c
    ../Src/Display/LCD/st7305_profile_fd042.c
    ../Src/Display/spi_display_bus.c)
target_include_directories(st7305_driver_test PRIVATE ../Inc)
target_compile_definitions(st7305_driver_test PRIVATE
    STM32TOOLS_DISPLAY_BACKEND_ST7305)
add_test(NAME st7305_driver_test COMMAND st7305_driver_test)

add_executable(mqtt_line_collector_test mqtt_line_collector_test.c
    ../Src/AT/ModuleFrameParser.c)
target_include_directories(mqtt_line_collector_test PRIVATE ../Inc)
add_test(NAME mqtt_line_collector_test COMMAND mqtt_line_collector_test)

# Generic storage transaction tests use I/O doubles, without board dependencies.
add_executable(dual_bank_transaction_test
    dual_bank_transaction_test.c ../Src/Flash/DualBankStore.c)
target_include_directories(dual_bank_transaction_test PRIVATE ../Inc)
add_test(NAME dual_bank_transaction_test COMMAND dual_bank_transaction_test)

add_executable(button_test
    ${CMAKE_CURRENT_SOURCE_DIR}/button_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Src/Button.c)
target_include_directories(button_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/button_stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/../Inc)
add_test(NAME button_test COMMAND button_test)

add_executable(st7305_lifecycle_test
    st7305_lifecycle_test.c
    ../Src/Display/LCD/lcd_st7305.c
    ../Src/Display/spi_display_bus.c)
target_include_directories(st7305_lifecycle_test PRIVATE ../Inc)
target_compile_definitions(st7305_lifecycle_test PRIVATE STM32TOOLS_DISPLAY_BACKEND_ST7305)
target_compile_options(st7305_lifecycle_test PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic;-UNDEBUG>)
add_test(NAME st7305_lifecycle_test COMMAND st7305_lifecycle_test)

add_library(api_binding_example OBJECT ../Examples/ApiUsage/api_binding_example.c)
target_include_directories(api_binding_example PRIVATE ../Inc button_stubs)
add_executable(api_contract_test api_contract_test.c)
target_include_directories(api_contract_test PRIVATE ../Inc button_stubs)
add_test(NAME api_contract_test COMMAND api_contract_test)

add_test(NAME host_registration_contract
    COMMAND "${CMAKE_COMMAND}" "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/registration-fixtures"
      -P "${CMAKE_CURRENT_LIST_DIR}/host_registration_contract.cmake")
add_executable(ml307_mqtt_control_test
    ml307_mqtt_control_test.c
    ../Src/ML307/ml307_mqtt.c
    ../Src/AT/at_codec.c
    ../Src/AT/ModuleFrameParser.c)
target_include_directories(ml307_mqtt_control_test PRIVATE ../Inc)
add_test(NAME ml307_mqtt_control_test COMMAND ml307_mqtt_control_test)

include("${CMAKE_CURRENT_LIST_DIR}/ConsolidatedTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/AuditTests.cmake")
stm32tools_finalize_host_tests()
