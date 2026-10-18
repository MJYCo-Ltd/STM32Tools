add_executable(ewm103_cwjap_fragment_test
    ewm103_cwjap_fragment_test.c
    ../Src/EWM103/ewm103.c ../Src/EWM103/ewm103_at.c
    ../Src/EWM103/ewm103_parser.c ../Src/AT/at_codec.c)
target_link_libraries(ewm103_cwjap_fragment_test PRIVATE module_frame_parser)
target_include_directories(ewm103_cwjap_fragment_test PRIVATE ../Inc)
target_compile_options(ewm103_cwjap_fragment_test PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic>)
add_test(NAME ewm103_cwjap_fragment_test COMMAND ewm103_cwjap_fragment_test)
