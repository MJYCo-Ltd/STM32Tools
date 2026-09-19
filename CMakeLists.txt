cmake_minimum_required(VERSION 3.22)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(STM32Tools LANGUAGES C)
endif()
# Source-interface targets deliberately compile in EACH consumer's context.
# App (USE_FREERTOS) and Bootloader must not share objects built with one
# another's HAL/RTOS/board defines. No product main.h or GPIO is selected here.
function(stm32tools_component name)
    add_library(stm32tools_${name} INTERFACE)
    add_library(STM32Tools::${name} ALIAS stm32tools_${name})
    target_include_directories(stm32tools_${name} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Inc")
    foreach(source IN LISTS ARGN)
        target_sources(stm32tools_${name} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    endforeach()
endfunction()
stm32tools_component(core Src/Common.c)
stm32tools_component(values Src/ValueFormat.c Src/Time/time_util.c)
stm32tools_component(at Src/AT/at_codec.c Src/AT/ModuleFrameParser.c Src/AT/AtSession.c)
stm32tools_component(http_range Src/Protocol/HttpRange.c)
stm32tools_component(storage
    Src/Flash/nor_flash.c Src/Flash/storage_backend.c Src/Flash/storage_partition.c
    Src/Flash/storage_record.c Src/Flash/storage_log.c Src/Flash/storage_firmware.c
    Src/Flash/storage_upgrade.c Src/Flash/storage_commit.c Src/Flash/storage_bank.c)
target_link_libraries(stm32tools_storage INTERFACE STM32Tools::core)
stm32tools_component(dual_bank Src/Flash/DualBankStore.c)
target_link_libraries(stm32tools_dual_bank INTERFACE STM32Tools::storage)
stm32tools_component(signed_firmware Src/Flash/SignedFirmware.c
    ThirdParty/monocypher/monocypher.c ThirdParty/monocypher/monocypher-ed25519.c)
target_include_directories(stm32tools_signed_firmware INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/monocypher")
stm32tools_component(st7305 Src/Display/LCD/lcd_st7305.c
    Src/Display/LCD/st7305_profile_fd042.c Src/Display/Graphics.c Src/Display/spi_display_bus.c)
target_compile_definitions(stm32tools_st7305 INTERFACE STM32TOOLS_DISPLAY_BACKEND_ST7305)
stm32tools_component(display_stm32 Src/Display/stm32_spi_display_bus.c)
stm32tools_component(auxiliary Src/Auxiliary.c)
stm32tools_component(uart_stm32 Src/UartReceive.c)
target_link_libraries(stm32tools_uart_stm32 INTERFACE STM32Tools::auxiliary)
stm32tools_component(button_stm32 Src/Button.c)
stm32tools_component(i2c_bus Src/Bus/i2c_bus.c)
target_link_libraries(stm32tools_i2c_bus INTERFACE STM32Tools::core)
stm32tools_component(i2c_stm32 Src/Bus/stm32_i2c_bus.c)
target_link_libraries(stm32tools_i2c_stm32 INTERFACE STM32Tools::i2c_bus)
stm32tools_component(aht20 Src/AHT20/aht20.c)
target_link_libraries(stm32tools_aht20 INTERFACE STM32Tools::i2c_bus)
stm32tools_component(tmp117 Src/TMP/tmp117.c)
target_link_libraries(stm32tools_tmp117 INTERFACE STM32Tools::i2c_bus)
stm32tools_component(w25q_stm32 Src/W25Q/w25q.c Src/W25Q/w25q_storage.c)
target_link_libraries(stm32tools_w25q_stm32 INTERFACE STM32Tools::storage)
stm32tools_component(modbus Src/Protocol/modbus_codec.c Src/Protocol/ModbusRtu.c
    Src/Protocol/ModbusMaster.c Src/Bus/Rs485Core.c Src/Bus/ModbusQueue.c)
target_link_libraries(stm32tools_modbus INTERFACE STM32Tools::core)
stm32tools_component(ml307 Src/ML307/ml307.c Src/ML307/ml307_at.c
    Src/ML307/ml307_parser.c Src/ML307/ml307_mqtt.c Src/ML307/ml307_http.c)
target_link_libraries(stm32tools_ml307 INTERFACE STM32Tools::at STM32Tools::core)
stm32tools_component(ewm103 Src/EWM103/ewm103.c Src/EWM103/ewm103_at.c Src/EWM103/ewm103_parser.c)
target_link_libraries(stm32tools_ewm103 INTERFACE STM32Tools::at STM32Tools::core)
stm32tools_component(iwdg_f4 Src/Bootloader/bootloader_iwdg.c)
stm32tools_component(bootloader_f4 Src/Bootloader/bootloader.c
    Src/Bootloader/bootloader_policy.c Src/Bootloader/bootloader_flash_stm32f4.c)
target_link_libraries(stm32tools_bootloader_f4 INTERFACE
    STM32Tools::iwdg_f4 STM32Tools::w25q_stm32 STM32Tools::storage)
