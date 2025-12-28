/*
 ******************************************************************************
 * @file           : mx22.h
 * @brief          : MX-22 模块驱动头文件
 ******************************************************************************
 *
 *  Created on: Dec 28, 2025
 *      Author: yty
 */
#ifndef __MX22_H
#define __MX22_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MX22_OK = 0,
    MX22_ERROR,
    MX22_TIMEOUT,
    MX22_NOT_READY,
} mx22_status_t;

/* ===== 初始化 ===== */
mx22_status_t MX22_Init(void);

/* ===== 模式控制 ===== */
void MX22_EnterCommandMode(void);
void MX22_EnterDataMode(void);

/* ===== AT 基础 ===== */
mx22_status_t MX22_Test(void); // AT
mx22_status_t MX22_GetVersion(char *buf, uint16_t len);

/* ===== 蓝牙参数 ===== */
mx22_status_t MX22_GetMAC(char *buf, uint16_t len);
mx22_status_t MX22_SetSPPName(const char *name);
mx22_status_t MX22_SetBLEName(const char *name);

/* ===== 串口 ===== */
mx22_status_t MX22_SetBaudrate(uint32_t baud);

/* ===== 连接状态 ===== */
bool MX22_IsConnected(void);

/* ===== 透传数据 ===== */
mx22_status_t MX22_SendData(uint8_t *data, uint16_t len);

#endif //* __MX22_H */
