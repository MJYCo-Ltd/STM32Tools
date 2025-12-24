/*
 * TMP117.c
 *
 *  Created on: Dec 24, 2025
 *      Author: yty
 */
#include "cmsis_os.h"
#include "tmp117.h"

// Internal: I2C write 16-bit MSB first (spec: MSB first)
static HAL_StatusTypeDef tmp117_tx_u16(tmp117_t *dev, uint8_t reg, uint16_t val) {
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (uint8_t)(val >> 8);
    buf[2] = (uint8_t)(val & 0xFF);
    return HAL_I2C_Master_Transmit(dev->hi2c, dev->i2c_addr7 << 1, buf, 3, HAL_MAX_DELAY);
}

// Internal: I2C read 16-bit MSB first: write pointer then read (Read Word timing)
static HAL_StatusTypeDef tmp117_rx_u16(tmp117_t *dev, uint8_t reg, uint16_t *val) {
    HAL_StatusTypeDef st;
    st = HAL_I2C_Master_Transmit(dev->hi2c, dev->i2c_addr7 << 1, &reg, 1, HAL_MAX_DELAY);
    if (st != HAL_OK) return st;
    uint8_t data[2];
    st = HAL_I2C_Master_Receive(dev->hi2c, dev->i2c_addr7 << 1, data, 2, HAL_MAX_DELAY);
    if (st != HAL_OK) return st;
    *val = ((uint16_t)data[0] << 8) | data[1];
    return HAL_OK;
}

HAL_StatusTypeDef tmp117_read_u16(tmp117_t *dev, uint8_t reg, uint16_t *val) {
    return tmp117_rx_u16(dev, reg, val);
}

HAL_StatusTypeDef tmp117_write_u16(tmp117_t *dev, uint8_t reg, uint16_t val) {
    return tmp117_tx_u16(dev, reg, val);
}

HAL_StatusTypeDef tmp117_read_temperature_raw(tmp117_t *dev, int16_t *raw) {
    uint16_t v;
    HAL_StatusTypeDef st = tmp117_rx_u16(dev, TMP117_REG_TEMP_RES, &v);
    if (st == HAL_OK) *raw = (int16_t)v;
    return st;
}

HAL_StatusTypeDef tmp117_read_temperature_c(tmp117_t *dev, float *degC) {
    int16_t raw;
    HAL_StatusTypeDef st = tmp117_read_temperature_raw(dev, &raw);
    if (st != HAL_OK) return st;
    // 1 LSB = 0.0078125 °C (spec)
    *degC = (float)raw * 0.0078125f;
    return HAL_OK;
}

HAL_StatusTypeDef tmp117_read_config(tmp117_t *dev, uint16_t *cfg) {
    return tmp117_rx_u16(dev, TMP117_REG_CONFIGURATION, cfg);
}

HAL_StatusTypeDef tmp117_write_config(tmp117_t *dev, uint16_t cfg) {
    return tmp117_tx_u16(dev, TMP117_REG_CONFIGURATION, cfg);
}

HAL_StatusTypeDef tmp117_set_mode(tmp117_t *dev, tmp117_mode_t mode) {
    uint16_t cfg;
    HAL_StatusTypeDef st = tmp117_read_config(dev, &cfg);
    if (st != HAL_OK) return st;
    cfg &= ~((uint16_t)(3U << TMP117_CFG_MOD0_Pos));    // clear MOD[1:0]
    cfg |= ((uint16_t)mode << TMP117_CFG_MOD0_Pos);
    return tmp117_write_config(dev, cfg);
}

HAL_StatusTypeDef tmp117_set_conv_avg(tmp117_t *dev, tmp117_conv_t conv, tmp117_avg_t avg) {
    uint16_t cfg;
    HAL_StatusTypeDef st = tmp117_read_config(dev, &cfg);
    if (st != HAL_OK) return st;
    cfg &= ~((uint16_t)(7U << TMP117_CFG_CONV0_Pos));   // clear CONV[2:0]
    cfg |= ((uint16_t)conv << TMP117_CFG_CONV0_Pos);
    cfg &= ~((uint16_t)(3U << TMP117_CFG_AVG0_Pos));    // clear AVG[1:0]
    cfg |= ((uint16_t)avg << TMP117_CFG_AVG0_Pos);
    return tmp117_write_config(dev, cfg);
}

HAL_StatusTypeDef tmp117_select_alert_mode(tmp117_t *dev, bool therm_mode, bool alert_pin_active_high, bool alert_pin_dataready) {
    uint16_t cfg;
    HAL_StatusTypeDef st = tmp117_read_config(dev, &cfg);
    if (st != HAL_OK) return st;
    // T/nA: 1=Therm, 0=Alert (spec)
    if (therm_mode) cfg |=  (1U << TMP117_CFG_TnA_Pos);
    else             cfg &= ~(1U << TMP117_CFG_TnA_Pos);
    // POL: 1=Active high (spec)
    if (alert_pin_active_high) cfg |=  (1U << TMP117_CFG_POL_Pos);
    else                       cfg &= ~(1U << TMP117_CFG_POL_Pos);
    // DR/Alert: 1=ALERT reflects Data_Ready (spec)
    if (alert_pin_dataready) cfg |=  (1U << TMP117_CFG_DR_ALERT_Pos);
    else                     cfg &= ~(1U << TMP117_CFG_DR_ALERT_Pos);
    return tmp117_write_config(dev, cfg);
}

HAL_StatusTypeDef tmp117_set_limits(tmp117_t *dev, int16_t high_raw, int16_t low_raw) {
    HAL_StatusTypeDef st = tmp117_tx_u16(dev, TMP117_REG_T_HIGH, (uint16_t)high_raw);
    if (st != HAL_OK) return st;
    return tmp117_tx_u16(dev, TMP117_REG_T_LOW, (uint16_t)low_raw);
}

HAL_StatusTypeDef tmp117_set_offset(tmp117_t *dev, int16_t offset_raw) {
    return tmp117_tx_u16(dev, TMP117_REG_TEMP_OFFSET, (uint16_t)offset_raw);
}

HAL_StatusTypeDef tmp117_read_device_id(tmp117_t *dev, uint16_t *did) {
    return tmp117_rx_u16(dev, TMP117_REG_DEVICE_ID, did);
}

HAL_StatusTypeDef tmp117_read_status_flags(tmp117_t *dev, bool *high_alert, bool *low_alert, bool *data_ready, bool *ee_busy) {
    uint16_t cfg;
    HAL_StatusTypeDef st = tmp117_read_config(dev, &cfg);
    if (st != HAL_OK) return st;
    if (high_alert) *high_alert = (cfg >> TMP117_CFG_HIGH_ALERT_Pos) & 1U;
    if (low_alert)  *low_alert  = (cfg >> TMP117_CFG_LOW_ALERT_Pos)  & 1U;
    if (data_ready) *data_ready = (cfg >> TMP117_CFG_DATA_READY_Pos) & 1U;
    if (ee_busy)    *ee_busy    = (cfg >> TMP117_CFG_EE_BUSY_Pos)    & 1U;
    return HAL_OK;
}

HAL_StatusTypeDef tmp117_eeprom_unlock(tmp117_t *dev, bool unlock) {
    // EUN (bit15) in EEPROM unlock register (0x04)
    uint16_t val = unlock ? (1U << 15) : 0U;
    return tmp117_tx_u16(dev, TMP117_REG_EE_UNLOCK, val);
}

HAL_StatusTypeDef tmp117_eeprom_busy(tmp117_t *dev, bool *busy) {
    uint16_t v;
    HAL_StatusTypeDef st = tmp117_rx_u16(dev, TMP117_REG_EE_UNLOCK, &v);
    if (st != HAL_OK) return st;
    // bit14 mirrors EEPROM_Busy (spec)
    if (busy) *busy = (v >> 14) & 1U;
    return HAL_OK;
}

HAL_StatusTypeDef tmp117_write_eeprom_gp(tmp117_t *dev, uint8_t ee_reg_05_06_08, uint16_t data) {
    // Unlock, write, poll busy (~7 ms typical), then general call reset to latch (spec)
    HAL_StatusTypeDef st;
    st = tmp117_eeprom_unlock(dev, true);
    if (st != HAL_OK) return st;

    st = tmp117_tx_u16(dev, ee_reg_05_06_08, data);
    if (st != HAL_OK) return st;

    // Poll busy flag (bit14) until 0
    uint32_t start = HAL_GetTick();
    while (1) {
        bool busy=false;
        st = tmp117_eeprom_busy(dev, &busy);
        if (st != HAL_OK) return st;
        if (!busy) break;
        if ((HAL_GetTick() - start) > 100) break; // guard
        osDelay(2);
    }
    // Lock happens automatically after general call reset; issue GC reset:
    st = tmp117_general_call_reset(dev->hi2c);
    return st;
}

HAL_StatusTypeDef tmp117_soft_reset(tmp117_t *dev) {
    // Write 1 to Soft_Reset bit (bit1) in configuration; duration ~2 ms (spec)
    uint16_t cfg;
    HAL_StatusTypeDef st = tmp117_read_config(dev, &cfg);
    if (st != HAL_OK) return st;
    cfg |= (1U << TMP117_CFG_SOFT_RESET_Pos);
    st = tmp117_write_config(dev, cfg);
    osDelay(3); // allow reset complete
    return st;
}

HAL_StatusTypeDef tmp117_general_call_reset(I2C_HandleTypeDef *hi2c) {
    // GC address 0x00, then second byte 0x06 per spec timing diagram
    uint8_t gc_addr = 0x00; // 7-bit general call
    uint8_t cmd[2] = {0x00, 0x06};
    // Two bytes following GC address; many HALs require addressing as 0x00<<1
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(hi2c, gc_addr << 1, cmd, 2, HAL_MAX_DELAY);
    osDelay(2); // device reset ~1.5 ms (spec)
    return st;
}

void TMP117_Init_Axilla(tmp117_t* pDev) {
  // 1. General Call Reset，确保加载EEPROM 配置
  tmp117_general_call_reset(pDev->hi2c);
  osDelay(2);
  // 2. 配置为一次转换模式 + 8 次平均
  uint16_t cfg;
  tmp117_read_config(pDev, &cfg);
  // 清除MOD、AVG、DR/Alert、POL 位
  cfg &= ~((3U << TMP117_CFG_MOD0_Pos) | (3U << TMP117_CFG_AVG0_Pos) |
           (1U << TMP117_CFG_DR_ALERT_Pos) | (1U << TMP117_CFG_POL_Pos));
  // 设置 MOD=11 → 一次转换模式
  cfg |= (TMP117_MODE_ONE_SHOT << TMP117_CFG_MOD0_Pos);
  // 设置 AVG=01 → 8 次平均
  cfg |= (TMP117_AVG_8 << TMP117_CFG_AVG0_Pos);
  // 设置 DR/Alert=1 → ALERT
  // 引脚作为数据就绪信号
  cfg |= (1U << TMP117_CFG_DR_ALERT_Pos);
  // 设置 POL=0 → ALERT 低电平有效
  cfg &= ~(1U << TMP117_CFG_POL_Pos);
  tmp117_write_config(pDev, cfg);
  // 3.偏移校准：腋下温度比口腔低约 0.5 ℃
  int16_t offset_raw = (int16_t)(0.5f / 0.0078125f); // ≈ 64 LSB
  tmp117_set_offset(pDev, offset_raw);
}

void TMP117_Read_Axilla(tmp117_t* pDev,float* pTc) {
  // 触发一次转换：设置 MOD=11
  tmp117_set_mode(pDev, TMP117_MODE_ONE_SHOT);
  // 等待 ALERT 引脚中断或轮询 DataReady 位
  bool dr = false;
  do {
    tmp117_read_status_flags(pDev, NULL, NULL, &dr, NULL);
    osDelay(100);
  } while (!dr);
  // 读取温度（读取后自动清除 DataReady）
  tmp117_read_temperature_c(pDev, pTc);
}
