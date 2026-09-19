# Call at the END of a host-test CMake entry, after adding all subdirectories.
# Test names match executable names; fixtures needing external inputs must register
# an explicit skip instead of disappearing from the test inventory.
include_guard(GLOBAL)
function(_stm32tools_check_test_directory directory)
  get_property(_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  get_property(_tests DIRECTORY "${directory}" PROPERTY TESTS)
  foreach(_target IN LISTS _targets)
    get_target_property(_type "${_target}" TYPE)
    if(_type STREQUAL "EXECUTABLE")
      if(NOT "${_target}" IN_LIST _tests)
        message(FATAL_ERROR "Host test executable '${_target}' has no same-name add_test(). Register it, including an explicit skip for required external fixtures.")
      endif()
      # These tests use assert(): Release must not silently compile checks away.
      target_compile_options("${_target}" PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-UNDEBUG>
        $<$<C_COMPILER_ID:MSVC>:/UNDEBUG>)
      if(TARGET stm32tools_dependency_check)
        add_dependencies("${_target}" stm32tools_dependency_check)
      endif()
    endif()
  endforeach()
  get_property(_children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(_child IN LISTS _children)
    _stm32tools_check_test_directory("${_child}")
  endforeach()
endfunction()
function(stm32tools_finalize_host_tests)
  _stm32tools_check_test_directory("${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()
