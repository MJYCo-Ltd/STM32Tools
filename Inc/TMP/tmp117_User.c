#include "TMP/tmp117.h"
#include "main.h"
#include "Auxiliary.h"

// I2C 句柄（用户需要在 main.c 或其他地方定义）
extern I2C_HandleTypeDef hi2c1;

static uint8_t TmpBuf[3];

static uint8_t I2C1_SEND_BUSY = 1;
static uint8_t I2C1_RECIVE_BUSY = 1;

// I2C 写函数（弱函数，用户可重写）
__attribute__((weak)) TMP117_Status TMP117_I2C_Write(uint8_t addr7, uint8_t reg,
                                                     uint16_t val, int type) {
  TmpBuf[0] = reg;
  TmpBuf[1] = (uint8_t)(val >> 8);
  TmpBuf[2] = (uint8_t)(val & 0xFF);
  HAL_StatusTypeDef status;

  switch (type) {
  case 1:
    I2C1_SEND_BUSY = 1;
    status = HAL_I2C_Master_Transmit_DMA(&hi2c1, addr7 << 1, TmpBuf, 3);
    while (status == HAL_OK && 1 == I2C1_SEND_BUSY) {
      Enter_Sleep();
    }
    break;
  case 2:
    I2C1_SEND_BUSY = 1;
    status = HAL_I2C_Master_Transmit_IT(&hi2c1, addr7 << 1, TmpBuf, 3);
    while (status == HAL_OK && 1 == I2C1_SEND_BUSY) {
      Enter_Sleep();
    }
    break;
  default:
    status =
        HAL_I2C_Master_Transmit(&hi2c1, addr7 << 1, TmpBuf, 3, HAL_MAX_DELAY);
    break;
  }

  return (status == HAL_OK) ? TMP117_OK : TMP117_ERR_WRITE_I2C;
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  I2C1_RECIVE_BUSY = 0;
}
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  I2C1_SEND_BUSY = 0;
}
// I2C 读函数（弱函数，用户可重写）
// 注意：TMP117 的温度相关寄存器值都是有符号的，读取后需要转换为 int16_t
__attribute__((weak)) TMP117_Status TMP117_I2C_Read(uint8_t addr7, uint8_t reg,
                                                    uint16_t *val, int type) {
  uint8_t data[2];
  HAL_StatusTypeDef status;
  switch (type) {
  case 1:
    I2C1_SEND_BUSY = 1;
    status = HAL_I2C_Master_Transmit_DMA(&hi2c1, addr7 << 1, &reg, 1);
    while (status == HAL_OK && 1 == I2C1_SEND_BUSY) {
      Enter_Sleep();
    }
    break;
  case 2:
    I2C1_SEND_BUSY = 1;
    status = HAL_I2C_Master_Transmit_IT(&hi2c1, addr7 << 1, &reg, 1);
    while (status == HAL_OK && 1 == I2C1_SEND_BUSY) {
      Enter_Sleep();
    }
    break;
  default:
    status =
        HAL_I2C_Master_Transmit(&hi2c1, addr7 << 1, &reg, 1, HAL_MAX_DELAY);
    break;
  }

  // 设置寄存器指针
  if (status != HAL_OK) {
    return TMP117_ERR_WRITE_I2C;
  }

  // 读取数据
  switch (type) {
  case 1:
    I2C1_RECIVE_BUSY = 1;
    status = HAL_I2C_Master_Receive_DMA(&hi2c1, addr7 << 1, data, 2);
    while (status == HAL_OK && 1 == I2C1_RECIVE_BUSY) {
      Enter_Sleep();
    }
    break;
  case 2:
    I2C1_RECIVE_BUSY = 1;
    status = HAL_I2C_Master_Receive_IT(&hi2c1, addr7 << 1, data, 2);
    while (status == HAL_OK && 1 == I2C1_RECIVE_BUSY) {
      Enter_Sleep();
    }
    break;
  default:
    status = HAL_I2C_Master_Receive(&hi2c1, addr7 << 1, data, 2, HAL_MAX_DELAY);
    break;
  }

  if (status != HAL_OK) {
    return TMP117_ERR_READ_I2C;
  }

  // 直接解释为有符号16位值（MSB 在前）
  *val = (data[0] << 8) | data[1];
  return TMP117_OK;
}
