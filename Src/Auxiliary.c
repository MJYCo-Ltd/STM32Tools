/*
 * Auxiliary.c
 *
 *  Created on: Apr 12, 2024
 *      Author: yty
 */
#include <string.h>
#include "UartReceive.h"
#include "cmsis_os.h"
#include "main.h"
#include "Auxiliary.h"

// TickType_t g_base;
#define FLASH_PARAM_PAGE_ADDR 0x0800FC00 // 最后一页
static HALF_WORD_DATA gJudgeEndian = {1}; /// 如果 gJudgeEndian.u8Data[0] == 1，说明是小端系统

STMSTATUS G_LOCAL = {0, 0, 0, 0};

/// 将输入字节数组逆序复制到输出缓冲区
void ReverseByteOrder(const uint8_t* pIn, uint8_t* pOut, uint8_t unLength);

/// 发送信息给串口
void SendDebugInfo(const uint8_t *pData, uint16_t uLength) {
  if (GetUartCount() < 1)
    return;
  UART_HandleTypeDef *pHUart = GetUart(1);
  while (HAL_OK != HAL_UART_Transmit(pHUart, pData, uLength, 30)) {
    osDelay(1);
  }
  UpdateUartSendInfo(pHUart, uLength);
}

extern TIM_HandleTypeDef htim10;
unsigned long g_TotalTime = 0;
void configureTimerForRunTimeStats(void) {
  HAL_TIM_Base_Start_IT(&htim10);
  g_TotalTime = 0;
}

unsigned long getRunTimeCounterValue(void) {
  return (g_TotalTime + __HAL_TIM_GET_COUNTER(&htim10));
}

/// 请求空间
void *RequestSpace(size_t unSize) {
  void *pBuffer = pvPortMalloc(unSize);
  if (NULL != pBuffer) {
    memset(pBuffer, 0, unSize);
  }

  return (pBuffer);
}

/// 回收空间
void RecycleSpace(void *pBuffer) { vPortFree(pBuffer); }

/// 保存数据到flash中
uint8_t SaveFlash(const uint8_t *pData, uint16_t unLength) {
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PageError = 0;

  // Step 1：解锁 Flash
  HAL_FLASH_Unlock();

  // Step 2：擦除
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_PARAM_PAGE_ADDR;
  EraseInitStruct.NbPages = 1;

  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
    // 擦除失败
    HAL_FLASH_Lock();
    return (0);
  }

  // Step 3：写入
  for (uint16_t i = 0; i < unLength; i += 2) {
    static uint16_t halfWord;
    memcpy(&halfWord, &pData[i], sizeof(halfWord));

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_PARAM_PAGE_ADDR + i,
                          halfWord) != HAL_OK) {
      HAL_FLASH_Lock();
      return (0);
    }
  }

  // Step 4：锁定
  HAL_FLASH_Lock();
  return (1);
}

/// 获取状态
STMSTATUS GetStatus(void) {
  G_LOCAL.unRamFree = xPortGetFreeHeapSize();
  //	S_LOCAL.unCPURate = GetCPUUsage();
  return (G_LOCAL);
}

/// 计算CRC
uint16_t CalCRC16(const uint8_t *buffer, uint16_t len) {
  uint16_t wcrc = 0XFFFF;
  uint8_t temp;
  static uint16_t i = 0, j = 0;
  for (i = 0; i < len; i++) {
    temp = *buffer & 0X00FF;
    buffer++;
    wcrc ^= temp;
    for (j = 0; j < 8; j++) {
      if (wcrc & 0X0001) {
        wcrc >>= 1;
        wcrc ^= 0XA001;
      } else {
        wcrc >>= 1;
      }
    }
  }
  return wcrc;
}

/// 为协议加上CRC校验
void AddCRC16(uint8_t *pBuffer, uint16_t unLen, uint8_t bIsLittleEndian) {
  HALF_WORD_DATA crc;
  crc.u16Data = CalCRC16(pBuffer, unLen - 2);
  ConvertHalfWord2LitteleEndian(&crc, pBuffer + unLen - 2);
}

/// 判断modbus的数据校验
uint8_t JudgeCRC16(const uint8_t *pBuffer, uint16_t unLen,
                   uint8_t bIsLittleEndian) {
  HALF_WORD_DATA crc;
  uint16_t localCrc = CalCRC16(pBuffer, unLen - 2);
  switch (bIsLittleEndian) {
  case 0:
    ConvertBigEndian2HalfWord(pBuffer + unLen - 2, &crc);
    break;
  case 1:
    ConvertLittleEndian2HalfWord(pBuffer + unLen - 2, &crc);
    break;
  default:
    return (0);
  }

  if (localCrc == crc.u16Data)
    return (1);
  else
    return (0);
}

void ConvertBigEndian2Double(const uint8_t* pData,DOUBLE_DATA* pDoubleOut) {
  if (gJudgeEndian.u8Data[0] == 1) {
    ReverseByteOrder(pData, pDoubleOut->u8Data, sizeof(DOUBLE_DATA));
  } else {
    memcpy(pDoubleOut, pData, sizeof(DOUBLE_DATA));
  }
}

void ConvertBigEndian2Word(const uint8_t* pData,WORD_DATA* pWordOut) {
  if (gJudgeEndian.u8Data[0] == 1) {
    ReverseByteOrder(pData, pWordOut->u8Data, sizeof(WORD_DATA));
  } else {
    memcpy(pWordOut, pData, sizeof(WORD_DATA));
  }
}

void ConvertBigEndian2HalfWord(const uint8_t* pData,HALF_WORD_DATA* pHalfWordOut) {
  if (gJudgeEndian.u8Data[0] == 1) {
    ReverseByteOrder(pData, pHalfWordOut->u8Data, sizeof(HALF_WORD_DATA));
  } else {
    memcpy(pHalfWordOut, pData, sizeof(HALF_WORD_DATA));
  }
}

void ConvertDouble2BigEndian(const DOUBLE_DATA* pDoubleData,uint8_t* pOutData) {
  if (gJudgeEndian.u8Data[0] == 1) {
    ReverseByteOrder(pDoubleData->u8Data, pOutData, sizeof(DOUBLE_DATA));
  } else {
    memcpy(pOutData, pDoubleData, sizeof(DOUBLE_DATA));
  }
}

void ConvertWord2BigEndian(const WORD_DATA* pWordData,uint8_t* pOutData) {
  if (gJudgeEndian.u8Data[0] == 1) {
    ReverseByteOrder(pWordData->u8Data, pOutData, sizeof(WORD_DATA));
  } else {
    memcpy(pOutData, pWordData, sizeof(WORD_DATA));
  }
}

void ConvertHalfWord2BigEndian(const HALF_WORD_DATA* pHalfWordData,uint8_t* pOutData) {
  if (gJudgeEndian.u8Data[0] == 1) {
    ReverseByteOrder(pHalfWordData->u8Data, pOutData, sizeof(HALF_WORD_DATA));
  } else {
    memcpy(pOutData, pHalfWordData, sizeof(HALF_WORD_DATA));
  }
}

void ConvertLittleEndian2Double(const uint8_t* pData,DOUBLE_DATA* pDoubleOut) {
  if (gJudgeEndian.u8Data[0] == 1) {
    memcpy(pDoubleOut, pData, sizeof(DOUBLE_DATA));
  } else {
    ReverseByteOrder(pData, pDoubleOut->u8Data, sizeof(DOUBLE_DATA));
  }
}

void ConvertLittleEndian2Word(const uint8_t* pData,WORD_DATA* pWordOut) {
  if (gJudgeEndian.u8Data[0] == 1) {
    memcpy(pWordOut, pData, sizeof(WORD_DATA));
  } else {
    ReverseByteOrder(pData, pWordOut->u8Data, sizeof(WORD_DATA));
  }
}

void ConvertLittleEndian2HalfWord(const uint8_t* pData,HALF_WORD_DATA* pHalfWordOut) {
  if (gJudgeEndian.u8Data[0] == 1) {
    memcpy(pHalfWordOut, pData, sizeof(HALF_WORD_DATA));
  } else {
    ReverseByteOrder(pData, pHalfWordOut->u8Data, sizeof(HALF_WORD_DATA));
  }
}

void ConvertDouble2LittleEndian(const DOUBLE_DATA* pDoubleData,uint8_t* pOutData) {
  if (gJudgeEndian.u8Data[0] == 1) {
    memcpy(pOutData, pDoubleData, sizeof(DOUBLE_DATA));
  } else {
    ReverseByteOrder(pDoubleData->u8Data, pOutData, sizeof(DOUBLE_DATA));
  }
}

void ConvertWord2LitteleEndian(const WORD_DATA* pWordData,uint8_t* pOutData) {
  if (gJudgeEndian.u8Data[0] == 1) {
    memcpy(pOutData, pWordData, sizeof(WORD_DATA));
  } else {
    ReverseByteOrder(pWordData->u8Data, pOutData, sizeof(WORD_DATA));
  }
}

void ConvertHalfWord2LitteleEndian(const HALF_WORD_DATA* pHalfWordData,uint8_t* pOutData) {
  if (gJudgeEndian.u8Data[0] == 1) {
    memcpy(pOutData, pHalfWordData, sizeof(HALF_WORD_DATA));
  } else {
    ReverseByteOrder(pHalfWordData->u8Data, pOutData, sizeof(HALF_WORD_DATA));
  }
}

/// 将输入字节数组逆序复制到输出缓冲区
void ReverseByteOrder(const uint8_t* pIn, uint8_t* pOut, uint8_t unLength) {
  if (pIn == NULL || pOut == NULL || unLength == 0) {
    return;
  }
  
  uint8_t i;
  // 从后往前复制到输出缓冲区
  for (i = 0; i < unLength; i++) {
    pOut[i] = pIn[unLength - 1 - i];
  }
}
