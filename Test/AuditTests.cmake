# Shared by the library's full entry and the product's isolated audit entry.
set(_tools_test "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(_tools "${_tools_test}/.." ABSOLUTE)
add_executable(mqtt_boundary_test "${_tools_test}/mqtt_boundary_test.c"
    "${_tools}/Src/ML307/ml307_mqtt.c" "${_tools}/Src/AT/at_codec.c"
    "${_tools}/Src/AT/ModuleFrameParser.c")
add_executable(button_edge_queue_test "${_tools_test}/button_edge_queue_test.c" "${_tools}/Src/Button.c")
target_include_directories(button_edge_queue_test PRIVATE "${_tools_test}/button_stubs")
add_executable(auxiliary_contract_test "${_tools_test}/auxiliary_contract_test.c" "${_tools}/Src/Auxiliary.c")
target_include_directories(auxiliary_contract_test PRIVATE "${_tools_test}/auxiliary_stubs")
target_compile_definitions(auxiliary_contract_test PRIVATE USE_FREERTOS)
add_executable(health_monitor_test "${_tools_test}/health_monitor_test.c")
foreach(_t IN ITEMS mqtt_boundary_test button_edge_queue_test auxiliary_contract_test health_monitor_test)
    target_include_directories(${_t} PRIVATE "${_tools}/Inc")
    target_compile_options(${_t} PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic;-UNDEBUG>)
    add_test(NAME ${_t} COMMAND ${_t})
endforeach()

add_test(NAME component_contract_test COMMAND "${CMAKE_COMMAND}"
    "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/component-contract"
    -P "${_tools_test}/component_contract_test.cmake")

add_executable(uart_budget_test "${_tools_test}/uart_budget_test.c" "${_tools}/Src/UartReceive.c")
target_include_directories(uart_budget_test PRIVATE "${_tools_test}/uart_stubs" "${_tools}/Inc")
target_compile_definitions(uart_budget_test PRIVATE USE_FREERTOS)
target_compile_options(uart_budget_test PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic;-UNDEBUG>)
add_test(NAME uart_budget_test COMMAND uart_budget_test)
