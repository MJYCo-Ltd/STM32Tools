#include <stdio.h>
#include <string.h>
#include "main.h"
#include "cmsis_os.h"
#include "epd_uc8253.h"
#include "Auxiliary.h"

#define SELECT_EPD HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define UNSELECT_EPD HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)

#define EPD_CMD HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET)
#define EPD_DATA HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, GPIO_PIN_SET)

#define EPD_ISBUSY (HAL_GPIO_ReadPin(BUSY_GPIO_Port, BUSY_Pin) == GPIO_PIN_RESET)

#define SCL_UNDER HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_RESET)
#define SCL_HIGH HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_SET)

#define SDA_UNDER HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_RESET)
#define SDA_HIGH HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET)

//extern SPI_HandleTypeDef hspi1;

// ===================== LUT 全刷新 =====================
static const uint8_t lut_full_update[] = {
    0x80, 0x48, 0x40, 0x00, 0x00, 0x00,
    0x40, 0x48, 0x80, 0x00, 0x00, 0x00,
    0x80, 0x48, 0x40, 0x00, 0x00, 0x00,
    0x40, 0x48, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00,
    0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00
};

// ===================== LUT 局部刷新 =====================
static const uint8_t lut_partial_update[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x13, 0x11, 0x00, 0x00, 0x00, 0x00,
    0x13, 0x11, 0x00, 0x00, 0x00, 0x00
};

//void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
//{
//    hspi->State = HAL_SPI_STATE_READY;
//}

//void SPI_Write(unsigned char value)
//{
//    uint8_t timeout = 3;
//    while (HAL_OK != HAL_SPI_Transmit(&hspi1, &value, 1, 1)&& timeout--)
//    {
//        osDelay(1);
//    }
//
//    if (timeout == 0)
//    {
//        SendDebugInfo("SPI TIMEOUT", 11);
//    }
//}

//void SPI_WriteBuffer(const unsigned char* pBuffer,uint16_t unLength)
//{
//    uint8_t timeout = 3;
//    while (HAL_OK != HAL_SPI_Transmit(&hspi1, pBuffer, unLength, 20)&& timeout--)
//    {
//        osDelay(1);
//    }
//
//    if (timeout == 0)
//    {
//        SendDebugInfo("Buffer TIMEOUT", 14);
//    }
//}

void SPI_Write(uint8_t dat)
{
    uint8_t i;
    SELECT_EPD;
    for(i=0;i<8;i++)
    {
        SCL_UNDER;
        if(dat&0x80)
        {
            SDA_HIGH;
        }
        else
        {
            SDA_UNDER;
        }
        SCL_HIGH;
        dat<<=1;
    }
    UNSELECT_EPD;
}
// ================= 低层通信函数 =================
void EPD_SendCommand(uint8_t cmd) {
    SELECT_EPD;
    EPD_CMD;
    SPI_Write(cmd);
    UNSELECT_EPD;
}

void EPD_SendData(uint8_t data) {
    SELECT_EPD;
    EPD_DATA;
    SPI_Write(data);
    UNSELECT_EPD;
}

void EPD_SendBuffer(const unsigned char* pBuffer,uint16_t unLength)
{
    SELECT_EPD;
    EPD_DATA;
    //SPI_WriteBuffer(pBuffer,unLength);
    UNSELECT_EPD;
}

void EPD_WaitUntilIdle(void)
{
    uint32_t timeout = 5000;
    while (EPD_ISBUSY && timeout--)
    {
        osDelay(1);
    }
    if (timeout == 0)
    {
        SendDebugInfo("BUSY TIMEOUT", 12);
    }
}

// ================= 电源管理 =================
void EPD_Wakeup(void) {
    EPD_SendCommand(EPD_CMD_POWER_ON); // Power ON
    EPD_WaitUntilIdle();
}

void EPD_Sleep(void) {
    EPD_SendCommand(EPD_CMD_POWER_OFF); // Power OFF
    EPD_SendCommand(EPD_CMD_DEEP_SLEEP); // Deep Sleep
    EPD_SendData(0xA5);
}

void EPD_LoadLUT(const uint8_t* lut, uint8_t is_partial) {
    if (is_partial) {
        EPD_SendCommand(EPD_CMD_W2W_LUT);  // 局刷 LUT 地址
    }
    else {
        EPD_SendCommand(EPD_CMD_VCOM_LUT);  // 全刷 LUT 地址
    }

    for (uint8_t i = 0; i < 42; i++) {
        EPD_SendData(lut[i]);
    }
}

// 全屏刷新
void EPD_DisplayFull(uint8_t* buffer) {
    EPD_LoadLUT(lut_full_update, 0);
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
    for (uint32_t i = 0; i < (EPD_WIDTH * EPD_HEIGHT) / 8; i++) {
        EPD_SendData(buffer[i]);
    }
    EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH);
}

// 局部刷新（快速，不闪屏）
void EPD_DisplayPartialBuffer(uint8_t* buffer) {
    EPD_LoadLUT(lut_partial_update, 1);

    EPD_SendCommand(EPD_CMD_PARTIAL_IN); // 进入局部模式
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
    for (uint32_t i = 0; i < (EPD_WIDTH * EPD_HEIGHT) / 8; i++) {
        EPD_SendData(buffer[i]);
    }
    EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH);
    EPD_SendCommand(EPD_CMD_PARTIAL_OUT); // 退出局部模式
}

// ================= 初始化 =================
void EPD_Init(void) {
    // 硬复位
    HAL_GPIO_WritePin(RES_GPIO_Port, RES_Pin, GPIO_PIN_RESET);
    osDelay(1);
    HAL_GPIO_WritePin(RES_GPIO_Port, RES_Pin, GPIO_PIN_SET);
    osDelay(1);
    EPD_WaitUntilIdle();


    // Power Setting
    // 使能内部电源，设定 VGH/VGL/VSH/VSL/VDHR
    //EPD_SendCommand(0x01);
    //EPD_SendData(0x03);   // VDS_EN=1, VDG_EN=1 (内部供电)
    //EPD_SendData(0x10);   // VGH/VGL = ±20V
    //EPD_SendData(0x3F);   // VSH = +15V
    //EPD_SendData(0x3F);   // VSL = -15V
    //EPD_SendData(0x0D);   // VDHR = +13V

    // Booster soft start
    //EPD_SendCommand(0x06);
    //EPD_SendData(0x17);
    //EPD_SendData(0x17);
    //EPD_SendData(0x17);

    // Panel Setting
    // Bit: [RES1 RES0 REG KW/R UD SHL SHD_N RST_N]
    // 设置分辨率类型、扫描方向、Booster ON
    EPD_SendCommand(EPD_CMD_PANEL_SETTING);
    EPD_SendData(0x1B);   // 240×416 + 反向扫描
    //EPD_SendData(0x8D);
    //EPD_SendData(0x0F);   // RES=11b → 480x240 模式；SHD_N=1，Booster ON
    //EPD_SendData(0x8D);   // 内部温度传感器、VCOM自动浮空

    // 分辨率设置 (0x61)
    // 数据格式: HRES[7:0], HRES[15:8], VRES[7:0], VRES[15:8]
//    EPD_SendCommand(EPD_CMD_RESOLUTION_SETTING);
//    EPD_SendData(0xF0);
//  EPD_SendData(1);
//  EPD_SendData(0xA0);

    // VCOM 与数据间隔设置 (CDI)
    //EPD_SendCommand(EPD_CMD_VCOM_AND_DATA_INTERVAL_SETTING);
    //EPD_SendData(0x97);   // 推荐值: VBD=11, DDX=01, CDI=0111

    // 使用内置温度传感器
    //EPD_SendCommand(EPD_CMD_TEMPERATURE_SENSOR_SELECTION);
    //EPD_SendData(0x00);
    
    //EPD_SendCommand(0x20);
    
    //EPD_SendCommand(0xE5);
    //EPD_SendData(0x5F);
    EPD_SendCommand(0xE0);
    EPD_SendData(0x02);
    EPD_SendCommand(0xE5);
    EPD_SendData(0x6E);
    // Power ON
    EPD_Wakeup();

    // 等待系统稳定
    osDelay(100);
}

extern uint8_t epd_frame[12480];
void EPD_Display_Clear(void)
{
    uint16_t i,j,Width,Height;
    Width=(EPD_WIDTH%8==0)?(EPD_WIDTH/8):(EPD_WIDTH/8+1);
    Height=EPD_HEIGHT;
    EPD_SendCommand(0x10);
    for (j=0;j<Height;j++)
    {
        for (i=0;i<Width;i++)
        {
            EPD_SendData(epd_frame[i+j*Width]);
        }
    }
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
    for (j=0;j<Height;j++)
    {
        for (i=0;i<Width;i++)
        {
            EPD_SendData(0xFF);
            epd_frame[i+j*Width]=0xFF;
        }
    }

}

// ================= 显示相关 =================
void EPD_Clear(void) {

    // 黑白屏为老数据
    // 黑白红屏为黑白数据
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_1);
    for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
        EPD_SendData(EPD_COLOR_WHITE);
    }
    EPD_WaitUntilIdle();

    // 在黑白屏此处为新数据，黑白红三色屏此处为红色的数据
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
    for (uint16_t i = 0; i < EPD_BUFFER_SIZE; i++) {
        EPD_SendData(EPD_COLOR_WHITE);
    }
    EPD_WaitUntilIdle();
    
    // 发送刷新屏幕命令
    EPD_DisplayFrame();
    EPD_WaitUntilIdle();
}

void EPD_Display(const uint8_t *image)
{
    uint16_t i,j,uWidth,uHeight;
    uWidth=(EPD_WIDTH%8==0)?(EPD_WIDTH/8):(EPD_WIDTH/8+1);
    uHeight=EPD_HEIGHT;
    EPD_SendCommand(0x10);
    for (j=0;j<uHeight;j++)
    {
        for (i=0;i<uWidth;i++)
        {
            EPD_SendData(epd_frame[i+j*uWidth]);
        }
    }
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
    for (j=0;j<uHeight;j++)
    {
        for (i=0;i<uWidth;i++)
        {
            EPD_SendData(image[i+j*uWidth]);
            epd_frame[i+j*uWidth]=image[i+j*uWidth];
        }
    }

}

void EPD_DisplayFrame(void) {
    EPD_Wakeup();
    EPD_WaitUntilIdle();
    EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH); // DISPLAY REFRESH
    EPD_WaitUntilIdle();
    osDelay(10);
}

// ================= 局部刷新 =================
void EPD_DisplayPartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data) {
    if ((x + w) > EPD_WIDTH || (y + h) > EPD_HEIGHT) return;

    uint16_t x_start = x & 0xFFF8; // 对齐到8位
    uint16_t x_end = x + w - 1;
    uint16_t y_start = y;
    uint16_t y_end = y + h - 1;

    // 进入部分刷新模式
    EPD_SendCommand(EPD_CMD_PARTIAL_IN); // partial in

    // 设置窗口
    EPD_SendCommand(EPD_CMD_PARTIAL_WINDOW);
    EPD_SendData((x_start >> 8) & 0xFF);
    EPD_SendData(x_start & 0xFF);
    EPD_SendData((x_end >> 8) & 0xFF);
    EPD_SendData(x_end & 0xFF);
    EPD_SendData((y_start >> 8) & 0xFF);
    EPD_SendData(y_start & 0xFF);
    EPD_SendData((y_end >> 8) & 0xFF);
    EPD_SendData(y_end & 0xFF);
    EPD_SendData(0x01); // enable

    // 写入图像数据
    EPD_SendCommand(EPD_CMD_DATA_START_TRANSMISSION_2);
    for (uint32_t i = 0; i < (w * h) / 8; i++) {
        EPD_SendData(data[i]);
    }

    // 刷新该区域
    EPD_SendCommand(EPD_CMD_DISPLAY_REFRESH);

    // 退出部分刷新模式
    EPD_SendCommand(EPD_CMD_PARTIAL_OUT);
}
