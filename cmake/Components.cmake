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

_stm32tools_component(Auxiliary Src/Auxiliary.c)
_stm32tools_component(UartSTM32F4 Src/UartReceive.c)
_stm32tools_component(ButtonSTM32F4 Src/Button.c)
_stm32tools_component(I2CSTM32F4 Src/Bus/stm32_i2c_bus.c)
_stm32tools_component(DisplaySTM32F4 Src/Display/stm32_spi_display_bus.c)
_stm32tools_component(PlatformSTM32F4)
target_link_libraries(stm32tools_UartSTM32F4 INTERFACE STM32Tools::Auxiliary)
target_link_libraries(stm32tools_I2CSTM32F4 INTERFACE STM32Tools::I2CBus)
target_link_libraries(stm32tools_PlatformSTM32F4 INTERFACE
    STM32Tools::Auxiliary STM32Tools::UartSTM32F4 STM32Tools::ButtonSTM32F4
    STM32Tools::I2CSTM32F4 STM32Tools::DisplaySTM32F4)

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

_stm32tools_component(Comm)

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

# Audit-branch target spelling aliases; all resolve to the canonical sources.
add_library(STM32Tools::core ALIAS stm32tools_Core)
add_library(STM32Tools::values ALIAS stm32tools_Utilities)
add_library(STM32Tools::at ALIAS stm32tools_AT)
add_library(STM32Tools::http_range ALIAS stm32tools_HttpRange)
add_library(STM32Tools::storage ALIAS stm32tools_Storage)
add_library(STM32Tools::dual_bank ALIAS stm32tools_Storage)
add_library(STM32Tools::signed_firmware ALIAS stm32tools_SignedFirmware)
add_library(STM32Tools::st7305 ALIAS stm32tools_ST7305)
add_library(STM32Tools::display_stm32 ALIAS stm32tools_DisplaySTM32F4)
add_library(STM32Tools::auxiliary ALIAS stm32tools_Auxiliary)
add_library(STM32Tools::uart_stm32 ALIAS stm32tools_UartSTM32F4)
add_library(STM32Tools::button_stm32 ALIAS stm32tools_ButtonSTM32F4)
add_library(STM32Tools::i2c_bus ALIAS stm32tools_I2CBus)
add_library(STM32Tools::i2c_stm32 ALIAS stm32tools_I2CSTM32F4)
add_library(STM32Tools::aht20 ALIAS stm32tools_AHT20)
add_library(STM32Tools::tmp117 ALIAS stm32tools_TMP117)
add_library(STM32Tools::w25q_stm32 ALIAS stm32tools_W25Q)
add_library(STM32Tools::modbus ALIAS stm32tools_Modbus)
add_library(STM32Tools::ml307 ALIAS stm32tools_ML307)
add_library(STM32Tools::ewm103 ALIAS stm32tools_EWM103)
add_library(STM32Tools::iwdg_f4 ALIAS stm32tools_IwdgSTM32F4)
add_library(STM32Tools::bootloader_f4 ALIAS stm32tools_BootloaderSTM32F4)
add_library(STM32Tools::health_monitor ALIAS stm32tools_HealthMonitor)
add_library(stm32tools_core ALIAS stm32tools_Core)
add_library(stm32tools_values ALIAS stm32tools_Utilities)
add_library(stm32tools_at ALIAS stm32tools_AT)
add_library(stm32tools_http_range ALIAS stm32tools_HttpRange)
add_library(stm32tools_storage ALIAS stm32tools_Storage)
add_library(stm32tools_dual_bank ALIAS stm32tools_Storage)
add_library(stm32tools_signed_firmware ALIAS stm32tools_SignedFirmware)
add_library(stm32tools_st7305 ALIAS stm32tools_ST7305)
add_library(stm32tools_display_stm32 ALIAS stm32tools_DisplaySTM32F4)
add_library(stm32tools_auxiliary ALIAS stm32tools_Auxiliary)
add_library(stm32tools_uart_stm32 ALIAS stm32tools_UartSTM32F4)
add_library(stm32tools_button_stm32 ALIAS stm32tools_ButtonSTM32F4)
add_library(stm32tools_i2c_bus ALIAS stm32tools_I2CBus)
add_library(stm32tools_i2c_stm32 ALIAS stm32tools_I2CSTM32F4)
add_library(stm32tools_aht20 ALIAS stm32tools_AHT20)
add_library(stm32tools_tmp117 ALIAS stm32tools_TMP117)
add_library(stm32tools_w25q_stm32 ALIAS stm32tools_W25Q)
add_library(stm32tools_modbus ALIAS stm32tools_Modbus)
add_library(stm32tools_ml307 ALIAS stm32tools_ML307)
add_library(stm32tools_ewm103 ALIAS stm32tools_EWM103)
add_library(stm32tools_iwdg_f4 ALIAS stm32tools_IwdgSTM32F4)
add_library(stm32tools_bootloader_f4 ALIAS stm32tools_BootloaderSTM32F4)
add_library(stm32tools_health_monitor ALIAS stm32tools_HealthMonitor)
