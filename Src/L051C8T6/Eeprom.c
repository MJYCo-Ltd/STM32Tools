/*
 ******************************************************************************
 * @file           : EEPROM.c
 * @brief          : Source file for TMP117.c file.
 *                   此文件为读取L051C8T6内置的EEPROM提供函数接口
 *                   0 表示没有 数据 0xff 表示有数据
 *                   字节操作，传入合理范围内的任何地址参数都可以；
 *                   半字操作，地址需要 2 字节对齐，就是 2的倍数；
 *                   全字操作，地址需要 4 字节对齐，就是 4 的倍数；
 ******************************************************************************
 *
 *  Created on: Dec 26, 2025
 *      Author: yty
 */

#include "L051C8T6/Eeprom.h"
#include "Auxiliary.h"
#include "main.h"
#include <string.h>

#define EEPROM_START_ADDRESS DATA_EEPROM_BASE
#define EEPROM_END_ADDRESS (DATA_EEPROM_END + 1)

#define DEFINE_FIND_ADDR_FUNC(name, type)                                      \
  uint32_t name(void) {                                                        \
    uint32_t step = sizeof(type);                                              \
    for (uint32_t addr = EEPROM_START_ADDRESS; addr < EEPROM_END_ADDRESS;      \
         addr += step) {                                                       \
      if (*(__IO type *)(addr) != 0) {                                         \
        return addr;                                                           \
      }                                                                        \
    }                                                                          \
    return Rand_range(EEPROM_START_ADDRESS, EEPROM_END_ADDRESS, step);         \
  }

DEFINE_FIND_ADDR_FUNC(FindBYTEAddr, uint8_t)
DEFINE_FIND_ADDR_FUNC(FindHALFWORDAddr, uint16_t)
DEFINE_FIND_ADDR_FUNC(FindWORDAddr, uint32_t)

#define FindcallAddr(call) Find##call##Addr
#define FLASH_TYPEPROGRAMDATA_call(call) FLASH_TYPEPROGRAMDATA_##call

#define DEFINE_WRITE_DATA(name, type, call)                                    \
  bool name(const type *pData, uint16_t length) {                              \
    if (pData == NULL || length == 0) {                                        \
      return false;                                                            \
    }                                                                          \
    uint32_t addr = FindcallAddr(call)();                                      \
    uint32_t numAddr = addr;                                                   \
    type nowLength = length + *(__IO type *)numAddr;                           \
    uint32_t align = sizeof(*pData);                                           \
    addr += (1 + *(__IO type *)addr) * align;                                  \
    if (addr >= EEPROM_END_ADDRESS) {                                          \
      addr -= EEPROM_START_ADDRESS;                                            \
    }                                                                          \
    if (HAL_OK != HAL_FLASHEx_DATAEEPROM_Unlock()) {                           \
      return (false);                                                          \
    }                                                                          \
    while (length > 0) {                                                       \
      if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_call(call),     \
                                         addr, *pData) != HAL_OK) {            \
        HAL_FLASHEx_DATAEEPROM_Lock();                                         \
        return false;                                                          \
      }                                                                        \
      if (*(__IO type *)addr != *pData) {                                      \
        HAL_FLASHEx_DATAEEPROM_Lock();                                         \
        return false;                                                          \
      }                                                                        \
      addr += align;                                                           \
      pData += 1;                                                              \
      length -= 1;                                                             \
      if (addr >= EEPROM_END_ADDRESS) {                                        \
        addr -= EEPROM_START_ADDRESS;                                          \
      }                                                                        \
    }                                                                          \
    if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_call(call),       \
                                       numAddr, nowLength) != HAL_OK) {        \
      HAL_FLASHEx_DATAEEPROM_Lock();                                           \
      return false;                                                            \
    }                                                                          \
    if (*(__IO type *)numAddr != nowLength) {                                  \
      HAL_FLASHEx_DATAEEPROM_Lock();                                           \
      return false;                                                            \
    }                                                                          \
    HAL_FLASHEx_DATAEEPROM_Lock();                                             \
    return true;                                                               \
  }

DEFINE_WRITE_DATA(EEPROM_WriteBytes, uint8_t, BYTE)
DEFINE_WRITE_DATA(EEPROM_WriteHalfWords, uint16_t, HALFWORD)
DEFINE_WRITE_DATA(EEPROM_WriteWords, uint32_t, WORD)

#define DEFINE_READ_DATA(name, type, call)                                     \
  bool name(type *pData, uint16_t *pLength) {                                  \
    if (pData == NULL || pLength == NULL) {                                    \
      return false;                                                            \
    }                                                                          \
    uint32_t addr = FindcallAddr(call)();                                      \
    uint32_t align = sizeof(*pData);                                           \
    type length = *(__IO type *)addr;                                          \
    *pLength = length;                                                         \
    addr += align;                                                             \
    if (addr >= EEPROM_END_ADDRESS) {                                          \
      addr -= EEPROM_START_ADDRESS;                                            \
    }                                                                          \
    for (uint16_t i = 0; i < length; ++i) {                                    \
      pData[i] = *(__IO type *)addr;                                           \
      addr += align;                                                           \
      if (addr >= EEPROM_END_ADDRESS) {                                        \
        addr -= EEPROM_START_ADDRESS;                                          \
      }                                                                        \
    }                                                                          \
    return true;                                                               \
  }

DEFINE_READ_DATA(EEPROM_ReadBytes, uint8_t, BYTE)
DEFINE_READ_DATA(EEPROM_ReadHalfWords, uint16_t, HALFWORD)
DEFINE_READ_DATA(EEPROM_ReadWords, uint32_t, WORD)

bool EEPROM_Erase(void) {
  if (HAL_FLASHEx_DATAEEPROM_Unlock() != HAL_OK) {
    return false;
  }

  HAL_FLASHEx_DATAEEPROM_EnableFixedTimeProgram();

  for (uint32_t addr = EEPROM_START_ADDRESS; addr < EEPROM_END_ADDRESS;
       addr += 4) {

    if (0x00000000 != *(__IO uint32_t *)addr &&
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, addr,
                                       0x00000000) != HAL_OK) {
      HAL_FLASHEx_DATAEEPROM_Lock();
      return false;
    }
  }

  HAL_FLASHEx_DATAEEPROM_Lock();
  return true;
}
