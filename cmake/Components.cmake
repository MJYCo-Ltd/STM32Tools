include_guard(GLOBAL)

# Source components are deliberately INTERFACE libraries: each firmware target
# compiles its ports with that firmware's HAL/RTOS/MCU flags. Do not reuse an App
# object library (USE_FREERTOS) in the bare-metal Bootloader.
function(_stm32tools_component name)
    add_library(stm32tools_${name} INTERFACE)
    add_library(STM32Tools::${name} ALIAS stm32tools_${name})
    get_filename_component(_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
    foreach(_source IN LISTS ARGN)
        target_sources(stm32tools_${name} INTERFACE "${_root}/${_source}")
    endforeach()
    target_include_directories(stm32tools_${name} INTERFACE "${_root}/Inc")
    target_compile_features(stm32tools_${name} INTERFACE c_std_11)
endfunction()

_stm32tools_component(Core
    Src/Common.c
)

_stm32tools_component(Utilities
    Src/ValueFormat.c
    Src/Time/time_util.c
)

_stm32tools_component(AT
    Src/AT/ModuleFrameParser.c
    Src/AT/at_codec.c
    Src/AT/AtSession.c
)

_stm32tools_component(Storage
    Src/Flash/nor_flash.c
    Src/Flash/storage_backend.c
    Src/Flash/storage_partition.c
    Src/Flash/storage_record.c
    Src/Flash/storage_log.c
    Src/Flash/storage_firmware.c
    Src/Flash/storage_upgrade.c
    Src/Flash/storage_commit.c
    Src/Flash/storage_bank.c
    Src/Flash/DualBankStore.c
)

_stm32tools_component(SignedFirmware
    Src/Flash/SignedFirmware.c
    ThirdParty/monocypher/monocypher.c
    ThirdParty/monocypher/monocypher-ed25519.c
)

_stm32tools_component(ST7305
    Src/Display/LCD/lcd_st7305.c
    Src/Display/LCD/st7305_profile_fd042.c
    Src/Display/Graphics.c
    Src/Display/spi_display_bus.c
)

_stm32tools_component(PlatformSTM32F4
    Src/Auxiliary.c
    Src/UartReceive.c
    Src/Button.c
    Src/Bus/stm32_i2c_bus.c
    Src/Display/stm32_spi_display_bus.c
)

_stm32tools_component(IwdgSTM32F4
    Src/Bootloader/bootloader_iwdg.c
)

_stm32tools_component(BootloaderSTM32F4
    Src/Bootloader/bootloader.c
    Src/Bootloader/bootloader_policy.c
    Src/Bootloader/bootloader_flash_stm32f4.c
)

_stm32tools_component(I2CBus
    Src/Bus/i2c_bus.c
)

_stm32tools_component(AHT20
    Src/AHT20/aht20.c
)

_stm32tools_component(TMP117
    Src/TMP/tmp117.c
)

_stm32tools_component(W25Q
    Src/W25Q/w25q.c
    Src/W25Q/w25q_storage.c
)

_stm32tools_component(Modbus
    Src/Protocol/modbus_codec.c
    Src/Bus/Rs485Core.c
    Src/Protocol/ModbusRtu.c
    Src/Protocol/ModbusMaster.c
    Src/Bus/ModbusQueue.c
)

_stm32tools_component(HttpRange
    Src/Protocol/HttpRange.c
)

_stm32tools_component(ML307
    Src/ML307/ml307.c
    Src/ML307/ml307_at.c
    Src/ML307/ml307_parser.c
    Src/ML307/ml307_mqtt.c
    Src/ML307/ml307_http.c
)

_stm32tools_component(EWM103
    Src/EWM103/ewm103.c
    Src/EWM103/ewm103_at.c
    Src/EWM103/ewm103_parser.c
)

_stm32tools_component(HealthMonitor
    Src/System/HealthMonitor.c
)

target_link_libraries(stm32tools_Utilities INTERFACE STM32Tools::Core)
target_link_libraries(stm32tools_Storage INTERFACE STM32Tools::Core)
target_link_libraries(stm32tools_PlatformSTM32F4 INTERFACE STM32Tools::IwdgSTM32F4 STM32Tools::I2CBus)
target_link_libraries(stm32tools_BootloaderSTM32F4 INTERFACE STM32Tools::Storage STM32Tools::Core STM32Tools::IwdgSTM32F4)
target_link_libraries(stm32tools_I2CBus INTERFACE STM32Tools::Core)
target_link_libraries(stm32tools_AHT20 INTERFACE STM32Tools::I2CBus)
target_link_libraries(stm32tools_TMP117 INTERFACE STM32Tools::I2CBus)
target_link_libraries(stm32tools_W25Q INTERFACE STM32Tools::Storage)
target_link_libraries(stm32tools_Modbus INTERFACE STM32Tools::Core)
target_link_libraries(stm32tools_HttpRange INTERFACE STM32Tools::Core)
target_link_libraries(stm32tools_ML307 INTERFACE STM32Tools::AT STM32Tools::Core)
target_link_libraries(stm32tools_EWM103 INTERFACE STM32Tools::AT STM32Tools::Core)
target_include_directories(stm32tools_SignedFirmware INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/monocypher")
target_compile_definitions(stm32tools_ST7305 INTERFACE STM32TOOLS_DISPLAY_BACKEND_ST7305)
