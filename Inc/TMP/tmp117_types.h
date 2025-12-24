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
#ifndef YTY_TMP117_TYPES_H
#define YTY_TMP117_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TMP117_OK = 0,
    TMP117_ERR_I2C,
    TMP117_ERR_TIMEOUT,
    TMP117_ERR_RANGE,
    TMP117_ERR_NOT_READY,
} tmp117_status_t;

/* 明确区分 raw / 摄氏度 */
typedef int16_t tmp117_raw_t;

typedef struct {
    float value;      // °C
    bool  valid;      // 是否在医学有效区间
} tmp117_temp_t;

#endif // YTY_TMP117_TYPES_H
