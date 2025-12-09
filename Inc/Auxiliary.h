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

/// 定义64位数据
typedef union{
    double dData;
    float fData[2];
    uint64_t u64Data;
    int64_t n64Data;
    uint32_t u32Data[2];
    int32_t n32Data[2];
    uint16_t u16Data[4];
    int16_t n16Data[4];
    uint8_t u8Data[8];
    int8_t n8Data[8];
}DOUBLE_DATA;

/// 定义32位数据
typedef union{
    float fData;
    uint32_t u32Data;
    int32_t n32Data;
    uint16_t u16Data[2];
    int16_t n16Data[2];
    uint8_t u8Data[4];
    int8_t n8Data[4];
}WORD_DATA;

/// 定义16位数据
typedef union{
    uint16_t u16Data;
    int16_t n16Data;
    uint8_t u8Data[2];
    int8_t n8Data[2];
    char cData[2];
    unsigned char ucData[2];
}HALF_WORD_DATA;

typedef struct {
    uint8_t unRamTotal;     /// ram总空间
    uint8_t unRamFree;      /// ram剩余空间
    uint8_t unCPURate;      /// CPU使用率
    uint8_t unCPUFrequency; /// CPU主频 MHZ
} STMSTATUS;

/// 接收数据
typedef struct {
    uint64_t unReciveCount; /// 接收到的数据总数 (字节)
    uint64_t unSendCount;   /// 发送的数据总数 (字节)
    uint64_t unDealCount;   /// 处理的数据总数 (字节)
} IOInfo;

/**
 * 发送调试信息
 *@pragma pData   要打印的字符串
 *@pragma unLength 要打印的字符串长度
 */
void SendDebugInfo(const uint8_t *pData, uint16_t unLength);

/**
 * 请求新的空间
 *@pragma unSize 要开辟空间的字节大小
 *@return 如果剩余空间大小小于申请的空间返回NULL
 *@attention 返回的空间都进行了置零操作
 */
void *RequestSpace(size_t unSize);

/**
 * 回收空间
 */
void RecycleSpace(void *pBuffer);

/**
 * @brief 向Flash中写入数据
 * @param pData 写入数据开始指针
 * @param unLength 写入数据长度（小于 1024）
 * @return 0 表示失败 1表示成功
 */
uint8_t SaveFlash(const uint8_t *pData, uint16_t unLength);

/**
 * @brief 获取flash
 * @return
 */
const uint8_t *ReadFlash();

/**
 *获取单片机状态
 */
STMSTATUS GetStatus(void);

/**
 * @brief 计算CRC值
 * @return
 */
uint16_t CalCRC16(const uint8_t *buffer, uint16_t len);

/**
 * @brief 给数据增加CRC校验
 * @param pBuffer 给定的数组
 * @param unLen   数组长度(unLen 往前的2位不参与CRC校验运算，并将存放 CRC校验)
 * @param bIsLittleEndian 0 表示大端 1 表示小端
 */
void AddCRC16(uint8_t *pBuffer, uint16_t unLen, uint8_t bIsLittleEndian);
/**
 * @brief 判断接收的数据的CRC16是否有效
 * @param bIsLittleEndian 0 表示大端 1 表示小端
 * @return 0 表示无效 1 表示有效
 */
uint8_t JudgeCRC16(const uint8_t *pBuffer, uint16_t unLen, uint8_t bIsLittleEndian);

/**
 * @brief 字节序转换 将大端的字节序的数值转换成 mcu一致的字节序的内容
 */
void ConvertBigEndian2Double(const uint8_t* pData,DOUBLE_DATA* pDoubleOut);
void ConvertBigEndian2Word(const uint8_t* pData,WORD_DATA* pWordOut);
void ConvertBigEndian2HalfWord(const uint8_t* pData,HALF_WORD_DATA* pHalfWordOut);

/**
 * @brief 字节序转换 将 mcu一致的字节序的内容转换成大端的字节序
 */
void ConvertDouble2BigEndian(const DOUBLE_DATA* pDoubleData,uint8_t* pOutData);
void ConvertWord2BigEndian(const WORD_DATA* pWordData,uint8_t* pOutData);
void ConvertHalfWord2BigEndian(const HALF_WORD_DATA* pHalfWordData,uint8_t* pOutData);

/**
 * @brief 字节序转换 将小端的字节序的数值转换成 mcu一致的字节序的数值
 */
void ConvertLittleEndian2Double(const uint8_t* pData,DOUBLE_DATA* pDoubleOut);
void ConvertLittleEndian2Word(const uint8_t* pData,WORD_DATA* pWordOut);
void ConvertLittleEndian2HalfWord(const uint8_t* pData,HALF_WORD_DATA* pHalfWordOut);

/**
 * @brief 字节序转换 将 mcu一致的字节序的内容转换成小端的字节序
 */
void ConvertDouble2LittleEndian(const DOUBLE_DATA* pDoubleData,uint8_t* pOutData);
void ConvertWord2LitteleEndian(const WORD_DATA* pWordData,uint8_t* pOutData);
void ConvertHalfWord2LitteleEndian(const HALF_WORD_DATA* pHalfWordData,uint8_t* pOutData);


#endif //__YTY_AUXILIARY_H
