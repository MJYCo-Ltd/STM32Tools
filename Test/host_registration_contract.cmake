cmake_minimum_required(VERSION 3.22)
if(NOT DEFINED TEST_WORK_DIR OR "${TEST_WORK_DIR}" STREQUAL "")
  message(FATAL_ERROR "TEST_WORK_DIR is required")
endif()
string(RANDOM LENGTH 10 ALPHABET 0123456789abcdef _id)
set(_root "${TEST_WORK_DIR}/${_id}")
get_filename_component(_helper "${CMAKE_CURRENT_LIST_DIR}/../cmake/HostTests.cmake" ABSOLUTE)
file(MAKE_DIRECTORY "${_root}/good" "${_root}/missing" "${_root}/nested/child")
set(_source "#include <assert.h>\n#ifdef NDEBUG\n#error assertions disabled\n#endif\nint main(void) { assert(1); return 0; }\n")
file(WRITE "${_root}/good/main.c" "${_source}")
file(WRITE "${_root}/good/CMakeLists.txt"
  "cmake_minimum_required(VERSION 3.22)\nproject(RegistrationContract C)\nenable_testing()\n"
  "include(\"${_helper}\")\nadd_executable(check main.c)\nadd_test(NAME check COMMAND check)\n"
  "stm32tools_finalize_host_tests()\n")
foreach(_config IN ITEMS Debug Release)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${_root}/good" -B "${_root}/build-${_config}"
    -G Ninja "-DCMAKE_BUILD_TYPE=${_config}"
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Configure failed: ${_out} ${_err}")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" --build "${_root}/build-${_config}"
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Assertions missing in ${_config}: ${_out} ${_err}")
  endif()
endforeach()
file(WRITE "${_root}/missing/main.c" "int main(void) { return 0; }\n")
file(WRITE "${_root}/missing/CMakeLists.txt"
  "cmake_minimum_required(VERSION 3.22)\nproject(Missing C)\nenable_testing()\n"
  "include(\"${_helper}\")\nadd_executable(forgotten main.c)\nstm32tools_finalize_host_tests()\n")
file(WRITE "${_root}/nested/child/main.c" "int main(void) { return 0; }\n")
file(WRITE "${_root}/nested/child/CMakeLists.txt" "add_executable(forgotten main.c)\n")
file(WRITE "${_root}/nested/CMakeLists.txt"
  "cmake_minimum_required(VERSION 3.22)\nproject(Nested C)\nenable_testing()\n"
  "include(\"${_helper}\")\nadd_subdirectory(child)\nstm32tools_finalize_host_tests()\n")
foreach(_bad IN ITEMS missing nested)
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${_root}/${_bad}" -B "${_root}/build-${_bad}" -G Ninja
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
  if(_result EQUAL 0 OR NOT "${_out}${_err}" MATCHES "no same-name add_test")
    message(FATAL_ERROR "Orphan test guard failed: ${_out} ${_err}")
  endif()
endforeach()
message(STATUS "Host registration contract: Debug/Release assertions and direct/nested orphan rejection passed")
