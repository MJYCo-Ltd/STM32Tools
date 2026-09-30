# Run the actual bootloader flow with mapped internal flash, durable-state and
# external-slot I/O doubles. The HAL stub traps the final application handoff.
add_executable(bootloader_install_test bootloader_install_test.c
    ../Src/Bootloader/bootloader.c ../Src/Bootloader/bootloader_policy.c
    ../Src/Common.c)
target_include_directories(bootloader_install_test PRIVATE bootloader_stubs ../Inc)
target_compile_definitions(bootloader_install_test PRIVATE
    STM32TOOLS_BOOT_CONFIG_HEADER="bootloader_test_config.h")
target_compile_options(bootloader_install_test PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Werror;-pedantic>)
add_test(NAME bootloader_install_test COMMAND bootloader_install_test)
