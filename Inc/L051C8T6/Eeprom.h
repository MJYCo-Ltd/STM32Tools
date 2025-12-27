/*
 ******************************************************************************
 * @file           : EEPROM.h
 * @brief          : Header for TMP117.c file.
 *                   此文件为读取L051C8T6内置的EEPROM提供函数接口
 ******************************************************************************
 *
 *  Created on: Dec 26, 2025
 *      Author: yty
 */
// eeprom.h
#ifndef YTY_EEPROM_H
#define YTY_EEPROM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 向EEPROM 写数据
 * @param data
 * @param length
 * @return
 */
bool EEPROM_WriteBytes(const uint8_t *data, uint16_t length);
bool EEPROM_WriteHalfWords(const uint16_t *data, uint16_t length);
bool EEPROM_WriteWords(const uint32_t *data, uint16_t length);

/**
 * @brief 从EEPROM读数据
 * @param data
 * @param length
 * @return
 */
bool EEPROM_ReadBytes(uint8_t *data, uint16_t* length);
bool EEPROM_ReadHalfWords(uint16_t *data, uint16_t* length);
bool EEPROM_ReadWords(uint32_t *data, uint16_t* length);

/**
 * @brief 清空数据
 * @return
 */
bool EEPROM_Erase();

#endif // YTY_EEPROM_H
