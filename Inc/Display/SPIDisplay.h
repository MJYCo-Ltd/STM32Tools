/*
 ******************************************************************************
 * @file           : SPIDisplay.h
 * @brief          : 此文件为Spi显示屏的统一接口
 ******************************************************************************
 *
 *  Created on: May 8, 2026
 *      Author: yty
 */
#ifndef __SPI_DISPLAY_H__
#define __SPI_DISPLAY_H__

static inline void SPI_SendCommand(uint8_t cmd) {
    SPI_SELECT;
    SPI_SEND_CMD;
    HAL_SPI_Transmit(&DISPLAY_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
    SPI_UNSELECT;
}

static inline void SPI_SendData(uint8_t data) {
    SPI_SELECT;
    SPI_SEND_DATA;
    HAL_SPI_Transmit(&DISPLAY_SPI_PORT, &data, 1, HAL_MAX_DELAY);
    SPI_UNSELECT;
}

static inline void SPI_SendBuffer(const uint8_t *buff, size_t buff_size) {
    SPI_SELECT;
    SPI_SEND_DATA;
    /* HAL 单次传输限制 64K，需分块发送 */
    while (buff_size > 0) {
        uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
#ifdef USE_BUFFER
        if (16 <= buff_size) {
            HAL_SPI_Transmit_DMA(&DISPLAY_SPI_PORT, buff, chunk_size);
            while (DISPLAY_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY) {
                osDelay(1);
            }
        } else {
            HAL_SPI_Transmit(&DISPLAY_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
        }
#else
        HAL_SPI_Transmit(&DISPLAY_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
#endif
        buff += chunk_size;
        buff_size -= chunk_size;
    }
    SPI_UNSELECT;
}
#endif
