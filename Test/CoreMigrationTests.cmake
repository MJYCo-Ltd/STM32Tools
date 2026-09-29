# Pure protocol tests: no Agriculture headers, HAL, RTOS or board globals.
target_link_libraries(ml307_http_test PRIVATE module_frame_parser)
add_executable(ml307_http_frame_test ml307_http_frame_test.c ../Src/ML307/ml307_http.c)
target_include_directories(ml307_http_frame_test PRIVATE ../Inc)
target_link_libraries(ml307_http_frame_test PRIVATE module_frame_parser)
target_compile_options(ml307_http_frame_test PRIVATE
    "$<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic>")
add_test(NAME ml307_http_frame_test COMMAND ml307_http_frame_test)
