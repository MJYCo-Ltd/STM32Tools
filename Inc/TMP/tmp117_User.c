#include "main.h"
#include "TMP/tmp117.h"

// I2C 句柄（用户需要在 main.c 或其他地方定义）
extern I2C_HandleTypeDef hi2c1;

// I2C 写函数（弱函数，用户可重写）
__attribute__((weak)) TMP117_Status TMP117_I2C_Write(uint8_t addr7, uint8_t reg, uint16_t val) {
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (uint8_t)(val >> 8);
    buf[2] = (uint8_t)(val & 0xFF);
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, addr7 << 1, buf, 3, HAL_MAX_DELAY);
    return (status == HAL_OK) ? TMP117_OK : TMP117_ERR_I2C;
}

// I2C 读函数（弱函数，用户可重写）
// 注意：TMP117 的温度相关寄存器值都是有符号的，读取后需要转换为 int16_t
__attribute__((weak)) TMP117_Status TMP117_I2C_Read(uint8_t addr7, uint8_t reg, uint16_t* val) {
    uint8_t data[2];
    
    // 设置寄存器指针
    if (HAL_I2C_Master_Transmit(&hi2c1, addr7 << 1, &reg, 1, HAL_MAX_DELAY) != HAL_OK) {
        return TMP117_ERR_I2C;
    }
    
    // 读取数据
    if (HAL_I2C_Master_Receive(&hi2c1, addr7 << 1, data, 2, HAL_MAX_DELAY) != HAL_OK) {
        return TMP117_ERR_I2C;
    }
    
    // 直接解释为有符号16位值（MSB 在前）
    *val = (data[0] << 8) | data[1];
    return TMP117_OK;
}
