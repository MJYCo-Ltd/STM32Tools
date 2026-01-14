/*
 ******************************************************************************
 * @file           : mx22_conf.h
 * @brief          : MX-22 模块配置文件
 ******************************************************************************
 *
 *  Created on: Dec 28, 2025
 *      Author: yty
 */
#ifndef __MX22_CONF_H
#define __MX22_CONF_H

#include "main.h" // 按你的芯片系列修改

/* ===== UART 句柄 ===== */
extern UART_HandleTypeDef huart1; // MX-22 UART

#define MX22_UART huart1
#define MX22_UART_TIMEOUT 5000 // ms

/* ===== 控制引脚（可选，但文档强烈建议接） ===== */

/* CDS：命令 / 数传模式切换
 * 高：数传模式
 * 低：命令模式
 */
#define MX22_CDS_PORT GPIOB
#define MX22_CDS_PIN GPIO_PIN_5

/* RST：低有效复位 */
#define MX22_RST_PORT GPIOB
#define MX22_RST_PIN GPIO_PIN_7

/* LINK：连接状态指示（输入） */
#define MX22_LINK_PORT GPIOB
#define MX22_LINK_PIN GPIO_PIN_4

#endif //__MX22_CONF_H
