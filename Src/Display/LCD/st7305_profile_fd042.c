#include "Display/LCD/lcd_st7305.h"

#define ST7305_SLP_OUT    0x11U
#define ST7305_INV_OFF    0x20U
#define ST7305_DISP_ON    0x29U
#define ST7305_TE_ON      0x35U
#define ST7305_MADCTL     0x36U
#define ST7305_HPM        0x38U
#define ST7305_DATA_FMT   0x3AU
#define ST7305_GATE_SET   0xB0U
#define ST7305_FRAME_RATE 0xB2U
#define ST7305_HPM_EQ     0xB3U
#define ST7305_LPM_EQ     0xB4U
#define ST7305_SOURCE_EQ  0xB7U
#define ST7305_PANEL_SET  0xB8U
#define ST7305_GAMMA      0xB9U
#define ST7305_CLEAR_RAM  0xBBU
#define ST7305_VG         0xC0U
#define ST7305_VSHP       0xC1U
#define ST7305_VSLP       0xC2U
#define ST7305_VSHN       0xC4U
#define ST7305_VSLN       0xC5U
#define ST7305_VSHL_SEL   0xC9U
#define ST7305_AUTO_PD    0xD0U
#define ST7305_BOOSTER    0xD1U
#define ST7305_NVM_LOAD   0xD6U
#define ST7305_OSC        0xD8U

static const uint8_t s_nvm_load[] = {0x13U, 0x02U};
static const uint8_t s_booster[] = {0x01U};
static const uint8_t s_gate_voltage[] = {0x12U, 0x0AU};
static const uint8_t s_vshp[] = {0x3CU, 0x3EU, 0x3CU, 0x3CU};
static const uint8_t s_vslp[] = {0x23U, 0x21U, 0x23U, 0x23U};
static const uint8_t s_vshn[] = {0x5AU, 0x5CU, 0x5AU, 0x5AU};
static const uint8_t s_vsln[] = {0x37U, 0x35U, 0x37U, 0x37U};
static const uint8_t s_oscillator[] = {0xA6U, 0xE9U};
static const uint8_t s_frame_rate[] = {0x12U};
static const uint8_t s_hpm_eq[] = {
    0xE5U, 0xF6U, 0x17U, 0x77U, 0x77U,
    0x77U, 0x77U, 0x77U, 0x77U, 0x71U};
static const uint8_t s_lpm_eq[] = {
    0x05U, 0x46U, 0x77U, 0x77U, 0x77U, 0x77U, 0x76U, 0x45U};
static const uint8_t s_gate_timing[] = {0x32U, 0x03U, 0x1FU};
static const uint8_t s_source_eq[] = {0x13U};
static const uint8_t s_gate_lines[] = {0x64U};
static const uint8_t s_vshl_select[] = {0x00U};
static const uint8_t s_madctl[] = {0x48U};
static const uint8_t s_data_format[] = {0x11U};
static const uint8_t s_gamma[] = {0x20U};
static const uint8_t s_panel[] = {0x29U};
static const uint8_t s_tear[] = {0x00U};
static const uint8_t s_auto_power_down[] = {0xFFU};
static const uint8_t s_clear_ram[] = {0x4FU};

static const SPI_DisplayCommand s_init_sequence[] = {
    {ST7305_NVM_LOAD, s_nvm_load, sizeof(s_nvm_load), 0U},
    {ST7305_BOOSTER, s_booster, sizeof(s_booster), 0U},
    {ST7305_VG, s_gate_voltage, sizeof(s_gate_voltage), 0U},
    {ST7305_VSHP, s_vshp, sizeof(s_vshp), 0U},
    {ST7305_VSLP, s_vslp, sizeof(s_vslp), 0U},
    {ST7305_VSHN, s_vshn, sizeof(s_vshn), 0U},
    {ST7305_VSLN, s_vsln, sizeof(s_vsln), 0U},
    {ST7305_OSC, s_oscillator, sizeof(s_oscillator), 0U},
    {ST7305_FRAME_RATE, s_frame_rate, sizeof(s_frame_rate), 0U},
    {ST7305_HPM_EQ, s_hpm_eq, sizeof(s_hpm_eq), 0U},
    {ST7305_LPM_EQ, s_lpm_eq, sizeof(s_lpm_eq), 0U},
    {0x62U, s_gate_timing, sizeof(s_gate_timing), 0U},
    {ST7305_SOURCE_EQ, s_source_eq, sizeof(s_source_eq), 0U},
    {ST7305_GATE_SET, s_gate_lines, sizeof(s_gate_lines), 0U},
    {ST7305_SLP_OUT, NULL, 0U, 120U},
    {ST7305_VSHL_SEL, s_vshl_select, sizeof(s_vshl_select), 0U},
    {ST7305_MADCTL, s_madctl, sizeof(s_madctl), 0U},
    {ST7305_DATA_FMT, s_data_format, sizeof(s_data_format), 0U},
    {ST7305_GAMMA, s_gamma, sizeof(s_gamma), 0U},
    {ST7305_PANEL_SET, s_panel, sizeof(s_panel), 0U},
    {ST7305_TE_ON, s_tear, sizeof(s_tear), 0U},
    {ST7305_AUTO_PD, s_auto_power_down, sizeof(s_auto_power_down), 0U},
    {ST7305_HPM, NULL, 0U, 0U},
    {ST7305_INV_OFF, NULL, 0U, 0U},
    {ST7305_CLEAR_RAM, s_clear_ram, sizeof(s_clear_ram), 0U},
    {ST7305_DISP_ON, NULL, 0U, 100U}};

const ST7305_PanelProfile ST7305_PANEL_FD042MN_ZF21_H06_B = {
    .width = ST7305_FD042MN_ZF21_H06_B_WIDTH,
    .height = ST7305_FD042MN_ZF21_H06_B_HEIGHT,
    .column_offset = 0x01U,
    .column_end = 0x2AU,
    .row_offset = 0x00U,
    .init_sequence = s_init_sequence,
    .init_sequence_count = sizeof(s_init_sequence) / sizeof(s_init_sequence[0])};
