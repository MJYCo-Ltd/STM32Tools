# Pure protocol tests: no Agriculture headers, HAL, RTOS or board globals.
target_link_libraries(ml307_http_test PRIVATE module_frame_parser)
add_executable(ml307_http_frame_test ml307_http_frame_test.c ../Src/ML307/ml307_http.c)
target_include_directories(ml307_http_frame_test PRIVATE ../Inc)
target_link_libraries(ml307_http_frame_test PRIVATE module_frame_parser)
target_compile_options(ml307_http_frame_test PRIVATE
    "$<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic>")
add_test(NAME ml307_http_frame_test COMMAND ml307_http_frame_test)

# Actual queue, metadata bank, record format and storage stack. Only NOR I/O
# and transaction callbacks are in-memory test doubles owned by each instance.
add_executable(durable_record_queue_test durable_record_queue_test.c
    ../Src/Flash/DurableRecordQueue.c ../Src/Flash/DualBankStore.c
    ../Src/Flash/storage_backend.c ../Src/Flash/storage_partition.c
    ../Src/Flash/storage_record.c ../Src/Flash/storage_bank.c
    ../Src/Flash/storage_commit.c ../Src/Flash/nor_flash.c ../Src/Common.c)
target_include_directories(durable_record_queue_test PRIVATE ../Inc)
target_compile_options(durable_record_queue_test PRIVATE
    "$<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror>")
add_test(NAME durable_record_queue_test COMMAND durable_record_queue_test)
