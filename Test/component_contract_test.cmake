cmake_minimum_required(VERSION 3.22)
if(NOT DEFINED TEST_WORK_DIR)
    message(FATAL_ERROR "TEST_WORK_DIR required")
endif()
get_filename_component(_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(MAKE_DIRECTORY "${TEST_WORK_DIR}")
file(WRITE "${TEST_WORK_DIR}/main.c" "#include <AHT20/aht20.h>\n#include <TMP/tmp117.h>\n#include <stddef.h>\nint main(void) { AHT20_Device a; TMP117_Device t; return AHT20_DeviceInit(&a,NULL,0x38,NULL,NULL)==AHT20_ERR_PARAM && TMP117_DeviceInit(&t,NULL,0x48)==TMP117_ERR_PARAM ? 0:1; }\n")
file(WRITE "${TEST_WORK_DIR}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.22)\nproject(ComponentContract C)\n"
    "add_subdirectory(\"${_root}\" tools)\n"
    "add_executable(contract main.c)\ntarget_link_libraries(contract PRIVATE STM32Tools::aht20 STM32Tools::tmp117)\n"
    "get_target_property(_kind stm32tools_auxiliary TYPE)\n"
    "if(NOT _kind STREQUAL \"INTERFACE_LIBRARY\")\nmessage(FATAL_ERROR \"HAL source must compile in the consumer context\")\nendif()\n"
    "enable_testing()\nadd_test(NAME contract COMMAND contract)\n")
# A real link with no HAL/main.h in the include path proves generic sensor
# components do not drag in STM32 ports and carry their common bus dependency.
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${TEST_WORK_DIR}" -B "${TEST_WORK_DIR}/build"
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "component configure: ${_out} ${_err}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${TEST_WORK_DIR}/build"
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "component build: ${_out} ${_err}")
endif()
execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${TEST_WORK_DIR}/build" --output-on-failure
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "component run: ${_out} ${_err}")
endif()
