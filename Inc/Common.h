/*
 ******************************************************************************
 * @file           : Common.h
 * @brief          : 此文件为通用函数的定义的辅助工具
 ******************************************************************************
 *
 *  Created on: Dec 10, 2025
 *      Author: yty
 */
#ifndef __YTY_COMMON_H_
#define __YTY_COMMON_H_
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

/**
 * @brief 获取随机地址
 * @param start 起始地址
 * @param end   结束地址
 * @param align 对齐方式
 * @attention range: [start, end)
 * @return
 */
uint32_t Rand_range(uint32_t start, uint32_t end, uint32_t align);

#endif //__YTY_COMMON_H_
