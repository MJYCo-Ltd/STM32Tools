/*
 ******************************************************************************
 * @file           : tmp117.h
 * @brief          : Header for tmp117.c file.
 *                   此文件为TMP117传感器的驱动
 ******************************************************************************
 *
 *  Created on: Dec 24, 2025
 *      Author: yty
 */
#ifndef __TMP117_H__
#define __TMP117_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief TMP117 I2C 7位地址定义（根据 ADD0 引脚配置，参考手册 Table 7-2）
 */
#define TMP117_ADDR_GND    (0x48)  ///< ADD0 连接到 GND 时的地址
#define TMP117_ADDR_VDD    (0x49)  ///< ADD0 连接到 VDD 时的地址
#define TMP117_ADDR_SDA    (0x4A)  ///< ADD0 连接到 SDA 时的地址
#define TMP117_ADDR_SCL    (0x4B)  ///< ADD0 连接到 SCL 时的地址

// 状态码定义
typedef enum {
    TMP117_OK = 0,
    TMP117_ERR_WRITE_I2C,
    TMP117_ERR_READ_I2C,
    TMP117_ERR_TIMEOUT,
    TMP117_ERR_RANGE,
    TMP117_ERR_NOT_READY,
} TMP117_Status;

// 工作模式
typedef enum {
    TMP117_MODE_CONTINUOUS = 0b00, /// 连续工作
    TMP117_MODE_SHUTDOWN   = 0b01, /// 关闭
    TMP117_MODE_ONE_SHOT   = 0b11, /// 获取一次关闭
} TMP117_Mode;

// 温度值结构体
typedef struct {
    float value;      // °C
    bool  valid;      // 是否在医学有效区间
} TMP117_Temp;

////////////////// 阻塞式 //////////////////////////
/**
 * @brief 获取 TMP117 温度值
 * @param addr7 I2C 7位地址（可使用 TMP117_ADDR_GND、TMP117_ADDR_VDD 等宏定义）
 * @param temp 温度值输出结构体指针
 * @return TMP117_Status 状态码，TMP117_OK 表示成功
 * @note 此函数会读取温度寄存器并转换为摄氏度，同时检查是否在医学有效区间（35-42°C）
 */
TMP117_Status TMP117_GetTemperature(uint8_t addr7, TMP117_Temp* temp);

/**
 * @brief 阻塞式 设置 TMP117 工作模式
 * @param addr7 I2C 7位地址（可使用 TMP117_ADDR_GND、TMP117_ADDR_VDD 等宏定义）
 * @param workMode 工作模式（TMP117_MODE_CONTINUOUS、TMP117_MODE_SHUTDOWN、TMP117_MODE_ONE_SHOT）
 * @return TMP117_Status 状态码，TMP117_OK 表示成功
 * @note 此函数会读取当前配置寄存器，修改模式位后写回
 */
TMP117_Status TMP117_SetWorkMode(uint8_t addr7, TMP117_Mode workMode);
////////////////// 阻塞式 end //////////////////////////

/////////////////// 非阻塞式 //////////////////////////

/////////////////// 非阻塞式 end //////////////////////
#endif // __TMP117_H__
