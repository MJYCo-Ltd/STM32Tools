/*
 * Common.c
 *
 *  Created on: Dec 10, 2025
 *      Author: yty
 */
#include <string.h>
#include "Common.h"
static HALF_WORD_DATA gJudgeEndian = {1}; /// 如果 gJudgeEndian.u8Data[0] == 1，说明是小端系统
/// 将输入字节数组逆序复制到输出缓冲区
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
