# STM32F411 Bootloader linker example

This is an illustrative 128 KiB Bootloader / 384 KiB Application layout for the
512 KiB STM32F411. It is not included by any STM32Tools CMake component.

A product must own and validate its linker files and supply
`STM32TOOLS_BOOT_CONFIG_HEADER` with matching Flash/SRAM macros. Do not treat
this example as the Agriculture memory-layout authority.
