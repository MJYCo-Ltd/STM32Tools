## 仓库说明

- 代码基于STM32CubeMX生成MDK工程
- FreeRTOS CMSIS_V2
::: warning
如果出现 链接错误 提示 SysTick_Handler 重定义，需要将FreeRTOSConfig.h 中的
``` 
//#define xPortSysTickHandler SysTick_Handler //注释掉即可
```
如果出现 CMSIS_device_header not defined
在出错的地方添加 
```
#ifndef CMSIS_device_header
#define CMSIS_device_header "stm32f1xx.h" // 只针对 1系列，cubemx对1系列很久不更新了
#endif
```
::: 
- 测试 MCU F103系列
- 生成的工程文件中的UserCodeBegin和UserCodeEnd中间引入代码，点击重新生成，不会覆盖已写的代码
# 串口通信
## cubemx中的设置
- 在DMA Settings 选项下增加 USART_RX 和 USART_TX
- 在NVIC Settings 选项下开启 global interrupt
## 性能
- 经过测试在目前 每个串口有100个字节的缓存区的情况下，每50毫秒，接收51个字节，不会丢失数据
# UC8253 支持
增加了对8253的支持，但目前还有错误
