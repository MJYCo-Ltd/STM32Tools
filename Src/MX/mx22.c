#include "MX/mx22.h"
#include "MX/mx22_conf.h"

#include <stdio.h>
#include <string.h>

/* ================= 内部配置 ================= */

#define MX22_RX_BUF_SIZE 256
#define MX22_AT_END "\r\n"

/* ================= 内部变量 ================= */

static char rx_buf[MX22_RX_BUF_SIZE];

/* ================= 内部函数 ================= */

/* UART 发送字符串 */
static mx22_status_t mx22_uart_send_str(const char *str) {
  if (HAL_UART_Transmit(&MX22_UART, (uint8_t *)str, strlen(str),
                        MX22_UART_TIMEOUT) != HAL_OK) {
    return MX22_ERROR;
  }
  return MX22_OK;
}

/* UART 接收（直到超时或缓冲满） */
static mx22_status_t mx22_uart_recv(char *buf, uint16_t len) {
  memset(buf, 0, len);

  if (HAL_UART_Receive(&MX22_UART, (uint8_t *)buf, len - 1,
                       MX22_UART_TIMEOUT) != HAL_OK) {
    return MX22_TIMEOUT;
  }
  return MX22_OK;
}

/* 发送 AT 并判断 OK */
static mx22_status_t mx22_send_at_ok(const char *cmd) {
  mx22_uart_send_str(cmd);

  if (mx22_uart_recv(rx_buf, sizeof(rx_buf)) != MX22_OK)
    return MX22_TIMEOUT;

  if (strstr(rx_buf, "OK"))
    return MX22_OK;

  return MX22_ERROR;
}

/* ================= 模式控制 ================= */

void MX22_EnterCommandMode(void) {
  /* CDS = LOW → Command mode */
  HAL_GPIO_WritePin(MX22_CDS_PORT, MX22_CDS_PIN, GPIO_PIN_RESET);
  HAL_Delay(10);
}

void MX22_EnterDataMode(void) {
  /* CDS = HIGH → Data mode */
  HAL_GPIO_WritePin(MX22_CDS_PORT, MX22_CDS_PIN, GPIO_PIN_SET);
  HAL_Delay(10);
}

/* ================= 初始化 ================= */

mx22_status_t MX22_Init(void) {
  /* 复位模块（文档：低有效，500ms） */
  HAL_GPIO_WritePin(MX22_RST_PORT, MX22_RST_PIN, GPIO_PIN_RESET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(MX22_RST_PORT, MX22_RST_PIN, GPIO_PIN_SET);

  /* 等待 READY */
  if (mx22_uart_recv(rx_buf, sizeof(rx_buf)) != MX22_OK)
    return MX22_TIMEOUT;

  if (strstr(rx_buf, "READY") == NULL)
    return MX22_NOT_READY;

  return MX22_OK;
}

/* ================= 基础 AT ================= */

mx22_status_t MX22_Test(void) {
  MX22_EnterCommandMode();
  return mx22_send_at_ok("AT\r\n");
}

mx22_status_t MX22_GetVersion(char *buf, uint16_t len) {
  MX22_EnterCommandMode();
  mx22_uart_send_str("AT+VER?\r\n");

  if (mx22_uart_recv(rx_buf, sizeof(rx_buf)) != MX22_OK)
    return MX22_TIMEOUT;

  if (strstr(rx_buf, "+VER:") == NULL)
    return MX22_ERROR;

  strncpy(buf, rx_buf, len - 1);
  return MX22_OK;
}

/* ================= 蓝牙参数 ================= */

mx22_status_t MX22_GetMAC(char *buf, uint16_t len) {
  MX22_EnterCommandMode();
  mx22_uart_send_str("AT+MAC?\r\n");

  if (mx22_uart_recv(rx_buf, sizeof(rx_buf)) != MX22_OK)
    return MX22_TIMEOUT;

  if (strstr(rx_buf, "+MAC:") == NULL)
    return MX22_ERROR;

  strncpy(buf, rx_buf, len - 1);
  return MX22_OK;
}

mx22_status_t MX22_SetSPPName(const char *name) {
  char cmd[64];

  if (!name || strlen(name) > 20)
    return MX22_ERROR;

  snprintf(cmd, sizeof(cmd), "AT+DNAME=%s\r\n", name);

  MX22_EnterCommandMode();
  return mx22_send_at_ok(cmd);
}

mx22_status_t MX22_SetBLEName(const char *name) {
  char cmd[64];

  if (!name || strlen(name) > 20)
    return MX22_ERROR;

  snprintf(cmd, sizeof(cmd), "AT+LENAME=%s\r\n", name);

  MX22_EnterCommandMode();
  return mx22_send_at_ok(cmd);
}

/* ================= 串口参数 ================= */

mx22_status_t MX22_SetBaudrate(uint32_t baud) {
  char cmd[48];

  /* 文档规定的合法波特率 */
  switch (baud) {
  case 9600:
  case 14400:
  case 19200:
  case 38400:
  case 57600:
  case 115200:
  case 230400:
  case 460800:
  case 921600:
    break;
  default:
    return MX22_ERROR;
  }

  snprintf(cmd, sizeof(cmd), "AT+URATE=%lu\r\n", baud);

  MX22_EnterCommandMode();
  return mx22_send_at_ok(cmd);
}

/* ================= 连接状态 ================= */

bool MX22_IsConnected(void) {
  /* LINK 高：已连接，低：未连接 */
  return (HAL_GPIO_ReadPin(MX22_LINK_PORT, MX22_LINK_PIN) == GPIO_PIN_SET);
}

/* ================= 数据透传 ================= */

mx22_status_t MX22_SendData(uint8_t *data, uint16_t len) {
  if (!data || len == 0)
    return MX22_ERROR;

  /* 文档：数传前必须进入 Data mode */
  MX22_EnterDataMode();

  if (HAL_UART_Transmit(&MX22_UART, data, len, MX22_UART_TIMEOUT) != HAL_OK) {
    return MX22_ERROR;
  }

  return MX22_OK;
}

/* 是否开启配对码 */
mx22_status_t MX22_EnablePairing(bool enable) {
  char cmd[32];

  snprintf(cmd, sizeof(cmd), "AT+PINE=%d\r\n", enable ? 1 : 0);

  MX22_EnterCommandMode();
  return mx22_send_at_ok(cmd);
}

mx22_status_t MX22_SetPairingPin(const char *pin) {
  char cmd[48];

  if (!pin || strlen(pin) == 0 || strlen(pin) > 16)
    return MX22_ERROR;

  snprintf(cmd, sizeof(cmd), "AT+PIN=%s\r\n", pin);

  MX22_EnterCommandMode();
  return mx22_send_at_ok(cmd);
}

mx22_status_t MX22_GetPairingPin(char *buf, uint16_t len) {
  MX22_EnterCommandMode();
  mx22_uart_send_str("AT+PIN?\r\n");

  if (mx22_uart_recv(rx_buf, sizeof(rx_buf)) != MX22_OK)
    return MX22_TIMEOUT;

  if (strstr(rx_buf, "+PIN:") == NULL)
    return MX22_ERROR;

  strncpy(buf, rx_buf, len - 1);
  return MX22_OK;
}

mx22_status_t MX22_WaitForConnection(uint32_t timeout_ms) {
  uint32_t start = HAL_GetTick();

  while ((HAL_GetTick() - start) < timeout_ms) {
    if (MX22_IsConnected())
      return MX22_OK;

    HAL_Delay(20);
  }

  return MX22_TIMEOUT;
}
