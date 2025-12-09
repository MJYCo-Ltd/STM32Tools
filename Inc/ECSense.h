/*
 ******************************************************************************
 * @file           : ECSense.h
 * @brief          : Header for ECSense.c file.
 *                   此文件为爱氪森传感器的驱动
 ******************************************************************************
 *
 *  Created on: Dec 8, 2025
 *      Author: yty
 */
#ifndef __YTY_ECSENSE_H_
#define __YTY_ECSENSE_H_

#include <stdint.h>

/// 传感器类型枚举
typedef enum {
    ECSENSE_HCHO = 0x17,      ///< 甲醛
    ECSENSE_VOC,              ///< 挥发性有机化合物
    ECSENSE_CO,               ///< 一氧化碳
    ECSENSE_Cl2,              ///< 氯气
    ECSENSE_H2,               ///< 氢气
    ECSENSE_H2S,              ///< 硫化氢
    ECSENSE_HCl,              ///< 氯化氢
    ECSENSE_HCN,              ///< 氰化氢
    ECSENSE_HF,               ///< 氟化氢
    ECSENSE_NH3,              ///< 氨气
    ECSENSE_NO2,              ///< 二氧化氮
    ECSENSE_O2,               ///< 氧气
    ECSENSE_O3,               ///< 臭氧
    ECSENSE_SO2,              ///< 二氧化硫
    ECSENSE_HBr,              ///< 溴化氢
    ECSENSE_Br2,              ///< 溴
    ECSENSE_F2,               ///< 氟气
    ECSENSE_PH3,              ///< 磷化氢
    ECSENSE_AsH3,             ///< 砷化氢
    ECSENSE_SiH4,             ///< 硅烷
    ECSENSE_GeH4,             ///< 锗烷
    ECSENSE_B2H6,             ///< 乙硼烷
    ECSENSE_BF3,              ///< 三氟化硼
    ECSENSE_WF6,              ///< 六氟化钨
    ECSENSE_SiF4,             ///< 四氟化硅
    ECSENSE_XeF2,             ///< 二氟化氙
    ECSENSE_TiF4,             ///< 四氟化钛
    ECSENSE_SMELL,            ///< 气味传感器
    ECSENSE_IAQ,              ///< 室内空气质量
    ECSENSE_AQI,              ///< 空气质量指数
    ECSENSE_NMHC,             ///< 非甲烷总烃
    ECSENSE_SOx,              ///< 硫氧化物
    ECSENSE_NOx,              ///< 氮氧化物
    ECSENSE_NO,               ///< 一氧化氮
    ECSENSE_C4H8,             ///< 丁烯
    ECSENSE_C3H8O2,           ///< 丙二醇
    ECSENSE_CH4S,             ///< 甲硫醇
    ECSENSE_C8H8,             ///< 苯乙烯
    ECSENSE_C4H10,            ///< 丁烷
    ECSENSE_C2H6,             ///< 乙烷
    ECSENSE_C6H14,            ///< 己烷
    ECSENSE_C2H4O,            ///< 乙醛
    ECSENSE_C3H9N,            ///< 三甲胺
    ECSENSE_C2H7N,            ///< 二甲胺
    ECSENSE_C2H6O,            ///< 乙醇
    ECSENSE_CS2,              ///< 二硫化碳
    ECSENSE_C2H6S,            ///< 乙硫醇
    ECSENSE_C2H6S2,           ///< 二乙基二硫
    ECSENSE_C2H4,             ///< 乙烯
    ECSENSE_CH3OH,            ///< 甲醇
    ECSENSE_C6H6,             ///< 苯
    ECSENSE_C8H10,            ///< 乙苯
    ECSENSE_C7H8,             ///< 甲苯
    ECSENSE_CH3COOH,          ///< 乙酸
    ECSENSE_ClO2,             ///< 二氧化氯
    ECSENSE_H2O2,             ///< 过氧化氢
    ECSENSE_N2H4,             ///< 肼
    ECSENSE_C2H8N2,           ///< 乙二胺
    ECSENSE_C2HCl3,           ///< 三氯乙烯
    ECSENSE_CHCl3,            ///< 氯仿
    ECSENSE_C2H3Cl3,          ///< 1,1,2-三氯乙烷
    ECSENSE_H2Se,             ///< 硒化氢
    ECSENSE_LEL,              ///< 爆炸下限
    ECSENSE_CO2,              ///< 二氧化碳
    ECSENSE_PID_VOCS,         ///< PID挥发性有机化合物
    ECSENSE_OTHERS = 0xD0     ///< 其他
} ECSense_DS4_Type;

/// 传感器单位类型
typedef enum{
    ECSENSE_PPM=0x02,
    ECSENSE_PPB=0x04,
    ECSENSE_VOL=0x08
}ECSense_DS4_Unit;

/// 指令类型枚举
typedef enum {
  ECSENSE_DS4_CMD_READ = 0x03,           /// 读取实时寄存器中的数
  ECSENSE_DS4_CMD_MODIFY_ADDR_RESP_PREFIX = 0xFF     ///< 修改Modbus地址指令
} ECSense_DS4_RecvType; /// 传感器返回类型

/// 传感器数据
typedef struct{
    uint8_t uAddr;              /// 传感器地址
    float fSmoothValue;         /// 处理过的数据
    uint16_t uMaxRange;         /// 最大量程
    ECSense_DS4_Type emGasType; ///类型
    ECSense_DS4_Unit emUnitType;/// 单位
    uint8_t uHealth;            /// 健康状态 0 健康 1 可更换 2 必须更换
    float fRealValue;           /// 真实数据
}ECSense_DS4_Value;

/**
 * @brief 构建修改Modbus地址的发送命令
 * @param newAddr 新的Modbus地址 (0x01 <= newAddr <= 0x7F)
 * @param pOutBuffer 输出缓冲区，至少需要14字节
 * @return 命令长度，如果地址无效返回0
 */
uint16_t ModifyAddr(uint8_t newAddr, uint8_t* pOutBuffer);

/**
 * @brief 解析修改Modbus地址的返回响应
 * @param pResponse 返回的响应数据
 * @param responseLen 响应数据长度
 * @param pAddr 输出解析得到的地址
 * @return 1表示成功，0表示失败
 */
uint8_t ModifyAddrResponse(const uint8_t* pResponse, uint16_t uResponseLen, uint8_t* pAddr);

/**
 * @brief 构建读取数据的命令
 * @param nAddr  指定地址的传感器
 * @param pOutBuffer 输出缓冲区，至少需要8字节
 * @return 命令长度，如果地址无效返回0
 */
uint16_t ReadDS4Value(uint8_t nAddr,uint8_t* pOutBuffer);
/**
 * @brief 读取数据命令
 * @param pHeader 输出缓冲区，用于存储响应头
 * @param headerSize 缓冲区大小
 * @param pEcsValue 传感器数据
 * @return 1表示成功，0表示失败
 */
uint8_t ReadDS4ValueResponse(const uint8_t *pResponse, uint16_t uResponseLen, ECSense_DS4_Value* pEcsValue);

#endif//__YTY_ECSENSE_H_
