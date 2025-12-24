/*
 ******************************************************************************
 * @file           : TMP117.h
 * @brief          : Header for TMP117.c file.
 *                   此文件为TMP117传感器的驱动
 ******************************************************************************
 *
 *  Created on: Dec 24, 2025
 *      Author: yty
 */
// tmp117.h
#ifndef YTY_TMP117_H
#define YTY_TMP117_H

#include "tmp117_types.h"

typedef struct {
    void* i2c;              // 不暴露 HAL_I2C_HandleTypeDef
    uint8_t addr7;
} tmp117_dev_t;

/* 生命周期 */
tmp117_status_t tmp117_init(tmp117_dev_t* dev);
tmp117_status_t tmp117_reset(tmp117_dev_t* dev);

/* 测量流程（状态机） */
tmp117_status_t tmp117_start_measure(tmp117_dev_t* dev);
tmp117_status_t tmp117_poll_ready(tmp117_dev_t* dev, bool* ready);
tmp117_status_t tmp117_read_temperature(tmp117_dev_t* dev,
                                        tmp117_temp_t* out);

/* 校准 */
tmp117_status_t tmp117_set_offset_raw(tmp117_dev_t* dev, tmp117_raw_t offset);

/* 设备信息 */
tmp117_status_t tmp117_read_device_id(tmp117_dev_t* dev, uint16_t* did);


#endif // YTY_TMP117_H
