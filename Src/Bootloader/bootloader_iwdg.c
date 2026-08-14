#include "Bootloader/bootloader_iwdg.h"
#include "Bootloader/bootloader_policy.h"

#include "stm32f4xx_hal.h"

#include <string.h>

static IWDG_HandleTypeDef g_iwdg;
static uint32_t g_reset_flags;

static uint32_t MapCsr(uint32_t csr)
{
  uint32_t flags = 0U;

  if ((csr & RCC_CSR_IWDGRSTF) != 0U) {
    flags |= BOOTLOADER_RST_IWDG;
  }
  if ((csr & RCC_CSR_PINRSTF) != 0U) {
    flags |= BOOTLOADER_RST_PIN;
  }
  if ((csr & RCC_CSR_PORRSTF) != 0U) {
    flags |= BOOTLOADER_RST_POR;
  }
  if ((csr & RCC_CSR_SFTRSTF) != 0U) {
    flags |= BOOTLOADER_RST_SFT;
  }
  return flags;
}

uint32_t BootloaderIwdg_CaptureResetFlags(void)
{
  g_reset_flags = MapCsr(RCC->CSR);
  return g_reset_flags;
}

void BootloaderIwdg_ClearResetFlags(void)
{
  __HAL_RCC_CLEAR_RESET_FLAGS();
}

uint32_t BootloaderIwdg_ResetFlags(void)
{
  return g_reset_flags;
}

uint8_t BootloaderIwdg_WasIwdgReset(void)
{
  return ((g_reset_flags & BOOTLOADER_RST_IWDG) != 0U) ? 1U : 0U;
}

void BootloaderIwdg_Init(void)
{
  /* Debugger halt must not look like an App hang. */
  __HAL_DBGMCU_FREEZE_IWDG();

  memset(&g_iwdg, 0, sizeof(g_iwdg));
  g_iwdg.Instance = IWDG;
  g_iwdg.Init.Prescaler = IWDG_PRESCALER_128;
  g_iwdg.Init.Reload = BOOTLOADER_IWDG_RELOAD;
  (void)HAL_IWDG_Init(&g_iwdg);
}

void BootloaderIwdg_Feed(void)
{
  /* Reload works whether this image started IWDG or the previous one did. */
  WRITE_REG(IWDG->KR, 0x0000AAAAu);
}

void BootloaderIwdg_SafeHold(void)
{
  __disable_irq();
  for (;;) {
    BootloaderIwdg_Feed();
  }
}
