# Distinct audit regressions on the consolidated implementations.
add_executable(mqtt_boundary_test mqtt_boundary_test.c
    ../Src/ML307/ml307_mqtt.c ../Src/AT/at_codec.c ../Src/AT/ModuleFrameParser.c)
add_executable(button_edge_queue_test button_edge_queue_test.c ../Src/Button.c)
target_include_directories(button_edge_queue_test PRIVATE button_stubs)
add_executable(auxiliary_contract_test auxiliary_contract_test.c ../Src/Auxiliary.c)
target_include_directories(auxiliary_contract_test PRIVATE auxiliary_stubs)
target_compile_definitions(auxiliary_contract_test PRIVATE USE_FREERTOS)
add_executable(health_audit_test health_audit_test.c ../Src/System/HealthMonitor.c)
foreach(_t IN ITEMS mqtt_boundary_test button_edge_queue_test auxiliary_contract_test health_audit_test)
    target_include_directories(${_t} PRIVATE ../Inc)
    target_compile_options(${_t} PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic;-UNDEBUG>)
    add_test(NAME ${_t} COMMAND ${_t})
endforeach()
add_test(NAME component_alias_contract_test COMMAND "${CMAKE_COMMAND}"
    "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/component-alias-contract"
    -P "${CMAKE_CURRENT_LIST_DIR}/component_contract_test.cmake")

# Compile the same real UART consumer with a non-default GLOBAL work budget.
add_executable(uart_budget_override_test uart_budget_test.c ../Src/UartReceive.c)
target_include_directories(uart_budget_override_test PRIVATE uart_stubs ../Inc)
target_compile_definitions(uart_budget_override_test PRIVATE USE_FREERTOS UART_RECEIVE_PROCESS_BUDGET=2)
target_compile_options(uart_budget_override_test PRIVATE $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic;-UNDEBUG>)
add_test(NAME uart_budget_override_test COMMAND uart_budget_override_test)
