#ifndef BOOTLOADER_TEST_HAL_H
#define BOOTLOADER_TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK = 0 } HAL_StatusTypeDef;
typedef struct { uint32_t CTRL, LOAD, VAL; } TestSysTick;
typedef struct { uint32_t ICSR, VTOR; } TestScb;
typedef struct { uint32_t ICER[8], ICPR[8]; } TestNvic;
extern TestSysTick test_systick;
extern TestScb test_scb;
extern TestNvic test_nvic;
#define SysTick (&test_systick)
#define SCB (&test_scb)
#define NVIC (&test_nvic)
#define SCB_ICSR_PENDSTCLR_Msk (1UL << 25)
#define SCB_ICSR_PENDSVCLR_Msk (1UL << 27)
void TestBootloader_Jump(void);
static inline HAL_StatusTypeDef HAL_RCC_DeInit(void) { return HAL_OK; }
static inline HAL_StatusTypeDef HAL_DeInit(void) { return HAL_OK; }
static inline void NVIC_SystemReset(void) { }
static inline void __disable_irq(void) { }
static inline void __enable_irq(void) { TestBootloader_Jump(); }
static inline void __set_BASEPRI(uint32_t value) { (void)value; }
static inline void __set_FAULTMASK(uint32_t value) { (void)value; }
static inline void __set_CONTROL(uint32_t value) { (void)value; }
static inline void __set_MSP(uint32_t value) { (void)value; }
static inline void __DSB(void) { }
static inline void __ISB(void) { }
#endif
