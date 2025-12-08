/*
 ******************************************************************************
 * @file           : Auxiliary.h
 * @brief          : Header for Auxiliary.c file.
 *                   此文件为STM32的辅助工具
 ******************************************************************************
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#ifndef __YTY_AUXILIARY_H_
#define __YTY_AUXILIARY_H_
#include <stddef.h>
#include <stdint.h>

typedef struct
{
	uint8_t unRamTotal;     /// ram总空间
    uint8_t unRamFree;      /// ram剩余空间
	uint8_t unCPURate;      /// CPU使用率
    uint8_t unCPUFrequency; /// CPU主频 MHZ
}STMSTATUS;

///接收数据
typedef struct
{
	uint64_t unReciveCount; /// 接收到的数据总数 (字节)
	uint64_t unSendCount;   /// 发送的数据总数 (字节)
	uint64_t unDealCount;   /// 处理的数据总数 (字节)
}IOInfo;

/**
 * 发送调试信息
 *@pragma pData   要打印的字符串
 *@pragma unLength 要打印的字符串长度
 */
void SendDebugInfo(const unsigned char* pData, uint16_t unLength);

/**
 * 请求新的空间
 *@pragma unSize 要开辟空间的字节大小
 *@return 如果剩余空间大小小于申请的空间返回NULL
 *@attention 返回的空间都进行了置零操作
 */
void* RequestSpace(size_t unSize);

/**
 * 回收空间
 */
void RecycleSpace(void* pBuffer);

/**
 * @brief 向Flash中写入数据
 * @param pData 写入数据开始指针
 * @param unLength 写入数据长度（小于 1024）
 * @return 0 表示失败 1表示成功
 */
uint8_t SaveFlash(const uint8_t* pData, uint16_t unLength);

/**
 * @brief 获取flash
 * @return
 */
const uint8_t* ReadFlash();

/**
 *获取单片机状态
 */
STMSTATUS GetStatus(void);

/**
 * @brief 计算CRC值
 * @param buffer
 * @param len
 * @return
 */
uint16_t CalCRC(const uint8_t *buffer, uint16_t len);

/**
 * @brief 判断接收的 modbus数据是否有效
 * @param pBuffer
 * @param unLen
 * @return
 */
uint8_t JudgeModbus(const uint8_t* pBuffer, uint16_t unLen);
#endif//__YTY_AUXILIARY_H
