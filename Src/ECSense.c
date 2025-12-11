/*
 * ECSense.c
 *
 *  Created on: Dec 8, 2025
 *      Author: yty
 */
#include <string.h>
#include <stdio.h>
#include "Common.h"
#include "ECSense.h"

/// 指令类型枚举
typedef enum {
  ECSENSE_DS4_CMD_READ = 0x03,                   /// 读取实时寄存器中的数据
  ECSENSE_DS4_CMD_WRITE = 0x10,                  /// 向寄存器中写入数据
  ECSENSE_DS4_CMD_MODIFY_ADDR_RESP_PREFIX = 0xFF ///< 修改Modbus地址指令
} ECSense_DS4_CMD_Type;                          /// 传感器返回类型

/// Modbus地址修改相关定义
#define MODBUS_ADDR_MIN 0x01     ///< 最小Modbus地址
#define MODBUS_ADDR_MAX 0x7F     ///< 最大Modbus地址

/// 修改地址命令头（发送）
static const uint8_t s_ModifyAddrCmdHeader[] = {0x80, 0x72, 0x65, 0x70, 0x6F,
                                                0x6C, 0x65, 0x76, 0x65, 0x44};
#define MODIFY_ADDR_CMD_HEADER_SIZE (sizeof(s_ModifyAddrCmdHeader))

/// 修改地址响应头（返回）
static const uint8_t s_ModifyAddrRespHeader[] = {
    ECSENSE_DS4_CMD_MODIFY_ADDR_RESP_PREFIX,
    0x72,
    0x65,
    0x70,
    0x6F,
    0x6C,
    0x65,
    0x76,
    0x65,
    0x44};
#define MODIFY_ADDR_RESP_HEADER_SIZE (sizeof(s_ModifyAddrRespHeader))

static const uint8_t s_ReadCmdHeader[] = {MODBUS_ADDR_MIN,ECSENSE_DS4_CMD_READ,0x20,0x00,0x00,0x08};
#define READ_CMD_HEADER_SIZE (sizeof(s_ReadCmdHeader))

static const uint8_t s_SleepCmd[]={MODBUS_ADDR_MIN,ECSENSE_DS4_CMD_WRITE,0x20,0x60,0x00,0x01,0x02,0x00,0x01};
#define SLEEP_HEADER_SIZE (sizeof(s_SleepCmd))

static const uint8_t s_WakeUpCmd[]={MODBUS_ADDR_MIN,ECSENSE_DS4_CMD_WRITE,0x20,0x70,0x00,0x02,0x04,0x00,0x01,0x00,0x01};
#define WAKEUP_HEADER_SIZE (sizeof(s_WakeUpCmd))

static const char* s_csUnKnow="UNKNOWN";
/// 传感器类型到气体字符串的查找表
static const struct {
  ECSense_DS4_Type type;
  const char *gasString;
} s_SensorGasStringMap[] = {{ECSENSE_HCHO, "HCHO"},
                            {ECSENSE_VOC, "VOC"},
                            {ECSENSE_CO, "CO"},
                            {ECSENSE_Cl2, "Cl2"},
                            {ECSENSE_H2, "H2"},
                            {ECSENSE_H2S, "H2S"},
                            {ECSENSE_HCl, "HCl"},
                            {ECSENSE_HCN, "HCN"},
                            {ECSENSE_HF, "HF"},
                            {ECSENSE_NH3, "NH3"},
                            {ECSENSE_NO2, "NO2"},
                            {ECSENSE_O2, "O2"},
                            {ECSENSE_O3, "O3"},
                            {ECSENSE_SO2, "SO2"},
                            {ECSENSE_HBr, "HBr"},
                            {ECSENSE_Br2, "Br2"},
                            {ECSENSE_F2, "F2"},
                            {ECSENSE_PH3, "PH3"},
                            {ECSENSE_AsH3, "AsH3"},
                            {ECSENSE_SiH4, "SiH4"},
                            {ECSENSE_GeH4, "GeH4"},
                            {ECSENSE_B2H6, "B2H6"},
                            {ECSENSE_BF3, "BF3"},
                            {ECSENSE_WF6, "WF6"},
                            {ECSENSE_SiF4, "SiF4"},
                            {ECSENSE_XeF2, "XeF2"},
                            {ECSENSE_TiF4, "TiF4"},
                            {ECSENSE_SMELL, "SMELL"},
                            {ECSENSE_IAQ, "IAQ"},
                            {ECSENSE_AQI, "AQI"},
                            {ECSENSE_NMHC, "NMHC"},
                            {ECSENSE_SOx, "SOx"},
                            {ECSENSE_NOx, "NOx"},
                            {ECSENSE_NO, "NO"},
                            {ECSENSE_C4H8, "C4H8"},
                            {ECSENSE_C3H8O2, "C3H8O2"},
                            {ECSENSE_CH4S, "CH4S"},
                            {ECSENSE_C8H8, "C8H8"},
                            {ECSENSE_C4H10, "C4H10"},
                            {ECSENSE_C2H6, "C2H6"},
                            {ECSENSE_C6H14, "C6H14"},
                            {ECSENSE_C2H4O, "C2H4O"},
                            {ECSENSE_C3H9N, "C3H9N"},
                            {ECSENSE_C2H7N, "C2H7N"},
                            {ECSENSE_C2H6O, "C2H6O"},
                            {ECSENSE_CS2, "CS2"},
                            {ECSENSE_C2H6S, "C2H6S"},
                            {ECSENSE_C2H6S2, "C2H6S2"},
                            {ECSENSE_C2H4, "C2H4"},
                            {ECSENSE_CH3OH, "CH3OH"},
                            {ECSENSE_C6H6, "C6H6"},
                            {ECSENSE_C8H10, "C8H10"},
                            {ECSENSE_C7H8, "C7H8"},
                            {ECSENSE_CH3COOH, "CH3COOH"},
                            {ECSENSE_ClO2, "ClO2"},
                            {ECSENSE_H2O2, "H2O2"},
                            {ECSENSE_N2H4, "N2H4"},
                            {ECSENSE_C2H8N2, "C2H8N2"},
                            {ECSENSE_C2HCl3, "C2HCl3"},
                            {ECSENSE_CHCl3, "CHCl3"},
                            {ECSENSE_C2H3Cl3, "C2H3Cl3"},
                            {ECSENSE_H2Se, "H2Se"},
                            {ECSENSE_LEL, "LEL"},
                            {ECSENSE_CO2, "CO2"},
                            {ECSENSE_PID_VOCS, "PID_VOCS"},
                            {ECSENSE_OTHERS, "OTHERS"}};

#define SENSOR_GAS_STRING_MAP_SIZE                                             \
  (sizeof(s_SensorGasStringMap) / sizeof(s_SensorGasStringMap[0]))

/// 传感器类型到气体字符串的查找表
static const struct {
  ECSense_DS4_Unit type;
  const char *unitString;
} s_SensorUnitStringMap[] = {
    {ECSENSE_PPM, "PPM"}, {ECSENSE_PPB, "PPB"}, {ECSENSE_VOL, "%VOL"}};
#define SENSOR_UNIT_STRING_MAP_SIZE                                            \
  (sizeof(s_SensorUnitStringMap) / sizeof(s_SensorUnitStringMap[0]))

/// 根据传感器类型获取对应的气体字符串
const char *ECSense_GetGasString(ECSense_DS4_Type type) {
  static uint8_t i;
  for (i = 0; i < SENSOR_GAS_STRING_MAP_SIZE; i++) {
    if (s_SensorGasStringMap[i].type == type) {
      return s_SensorGasStringMap[i].gasString;
    }
  }
  return(s_csUnKnow); // 无效类型
}

/// 根据传感器单位类型获取对应的字符串
const char *ECSense_GetUnitString(ECSense_DS4_Unit unitType) {
  static uint8_t i;
  for (i = 0; i < SENSOR_UNIT_STRING_MAP_SIZE; i++) {
    if (s_SensorUnitStringMap[i].type == unitType) {
      return s_SensorUnitStringMap[i].unitString;
    }
  }
  return(s_csUnKnow); // 无效类型
}

/// 构建修改Modbus地址的发送命令
uint16_t ModifyAddr(uint8_t newAddr, uint8_t *pOutBuffer) {
  if (pOutBuffer == NULL) {
    return 0;
  }

  // 验证地址范围
  if (newAddr < MODBUS_ADDR_MIN || newAddr > MODBUS_ADDR_MAX) {
    return 0;
  }

  // 1. 复制指令头
  memcpy(pOutBuffer, s_ModifyAddrCmdHeader, MODIFY_ADDR_CMD_HEADER_SIZE);
  uint16_t index = MODIFY_ADDR_CMD_HEADER_SIZE;

  // 2. 添加地址
  pOutBuffer[index] = newAddr;
  index += 3;

  // 3. 计算CRC（对指令头+地址计算CRC）
  AddCRC16(pOutBuffer, index, 1);
  return (index); // 返回总长度
}

/// 解析修改Modbus地址的返回响应
uint8_t ModifyAddrResponse(const uint8_t *pResponse, uint16_t uResponseLen,
                           uint8_t *pAddr) {
  /// 检查最小长度：指令头(10) + 地址(1) + CRC(2)
  if (uResponseLen < (MODIFY_ADDR_RESP_HEADER_SIZE + 3)) {
    return 0;
  }

  // 1. 检查响应头
  if (memcmp(pResponse, s_ModifyAddrRespHeader, MODIFY_ADDR_RESP_HEADER_SIZE) !=
      0) {
    return 0; // 响应头不匹配
  }

  // 2. 提取地址
  uint8_t addr = pResponse[MODIFY_ADDR_RESP_HEADER_SIZE];

  // 3. 验证地址范围
  if (addr < MODBUS_ADDR_MIN || addr > MODBUS_ADDR_MAX) {
    return 0;
  }

  // 4. 验证CRC（对指令头+地址计算CRC）
  if (0 == JudgeCRC16(pResponse, MODIFY_ADDR_RESP_HEADER_SIZE + 3, 1)) {
    return 0; // CRC校验失败
  }

  // 5. 返回地址
  *pAddr = addr;
  return 1; // 成功
}

/// 获取读取DS4值的缓存
uint16_t ReadDS4Value(uint8_t nAddr, uint8_t *pOutBuffer) {
  // 验证地址范围
  if (nAddr < MODBUS_ADDR_MIN || nAddr > MODBUS_ADDR_MAX) {
    return 0;
  }

  memcpy(pOutBuffer, s_ReadCmdHeader, READ_CMD_HEADER_SIZE);
  uint16_t index = READ_CMD_HEADER_SIZE + 2;
  pOutBuffer[0] = nAddr;
  AddCRC16(pOutBuffer, index, 1);

  return (index);
}

uint8_t ReadDS4ValueResponse(const uint8_t *pResponse, uint16_t uResponseLen,
                             ECSense_DS4_Value *pEcsValue) {
  if (uResponseLen < 21) {
    return (0);
  }
  pEcsValue->uAddr = pResponse[0];
  if (pEcsValue->uAddr < MODBUS_ADDR_MIN ||
      pEcsValue->uAddr > MODBUS_ADDR_MAX) {
    return (0);
  }

  if (ECSENSE_DS4_CMD_READ != pResponse[1]) {
    return (0);
  }

  if (0 == JudgeCRC16(pResponse, 21, 1)) {
    return (0); // CRC校验失败
  }

  WORD_DATA data;
  ConvertBigEndian2Word(pResponse + 3, &data);
  pEcsValue->fSmoothValue = data.fData;

  HALF_WORD_DATA halfData;
  ConvertBigEndian2HalfWord(pResponse + 7, &halfData);
  pEcsValue->uMaxRange = halfData.n16Data;

  pEcsValue->emGasType = pResponse[10];
  pEcsValue->emUnitType = pResponse[12];

  pEcsValue->uHealth = pResponse[14];
  ConvertBigEndian2Word(pResponse + 15, &data);
  pEcsValue->fRealValue = data.fData;

  return (1);
}

uint16_t DS4Sleep(uint8_t nAddr,uint8_t* pOutBuffer){
  if (nAddr < MODBUS_ADDR_MIN || nAddr > MODBUS_ADDR_MAX) {
    return 0;
  }

  memcpy(pOutBuffer, s_SleepCmd, SLEEP_HEADER_SIZE);
  uint16_t index = SLEEP_HEADER_SIZE + 2;
  pOutBuffer[0] = nAddr;
  AddCRC16(pOutBuffer, index, 1);

  return (index);
}

uint16_t DS4Wakeup(uint8_t nAddr,uint8_t* pOutBuffer){
  if (nAddr < MODBUS_ADDR_MIN || nAddr > MODBUS_ADDR_MAX) {
    return 0;
  }

  memcpy(pOutBuffer, s_WakeUpCmd, WAKEUP_HEADER_SIZE);
  uint16_t index = WAKEUP_HEADER_SIZE + 2;
  pOutBuffer[0] = pOutBuffer[8] = nAddr;
  AddCRC16(pOutBuffer, index, 1);

  return (index);
}

/// 将数据转换成可读的信息
uint16_t GetShowInfo(const ECSense_DS4_Value *pDS4Value, char *pBuffer) {
  static int nScale = 1000;
  return sprintf(pBuffer,
                 "Addr: %d;Gas: %s;SValue: %d.%d%s;MaxValue: %d;Health: "
                 "%d;RValue: %d.%d%s",
                 pDS4Value->uAddr, ECSense_GetGasString(pDS4Value->emGasType),
                 (int)(pDS4Value->fSmoothValue * nScale) / nScale,
                 (int)(pDS4Value->fSmoothValue * nScale) % nScale,
                 ECSense_GetUnitString(pDS4Value->emUnitType),
                 pDS4Value->uMaxRange, pDS4Value->uHealth,
                 (int)(pDS4Value->fRealValue * nScale) / nScale,
                 (int)(pDS4Value->fRealValue * nScale) % nScale,
                 ECSense_GetUnitString(pDS4Value->emUnitType));
}
