/* This executable links the real named portable components, without any HAL,
 * main.h, product include path or test-double implementation. */
#include <AT/at_codec.h>
#include <ML307/ml307.h>
#include <ML307/ml307_mqtt.h>
#include <AHT20/aht20.h>
#include <TMP/tmp117.h>
#include <System/HealthMonitor.h>
#include <assert.h>
#include <string.h>
int main(void) {
  char command[64];
  ML307_MqttEvent event;
  assert(ML307_MqttBuildDisconnect(command, sizeof(command), 0U) == ML307_RESULT_OK);
  assert(strcmp(command, "AT+MQTTDISC=0\r\n") == 0);
  assert(ML307_MqttParseUrc("+MQTTURC: \"suback\",0,42,1", &event) == ML307_RESULT_OK);
  assert(event.message_id == 42U);
  AHT20_Device aht;
  TMP117_Device tmp;
  assert(AHT20_DeviceInit(&aht, NULL, AHT20_I2C_ADDR7, NULL, NULL) == AHT20_ERR_PARAM);
  assert(TMP117_DeviceInit(&tmp, NULL, TMP117_ADDR_GND) == TMP117_ERR_PARAM);
  HealthMonitor monitor;
  HealthMonitor_Init(&monitor);
  assert(HealthMonitor_Register(&monitor, 0U, 100U, 0U));
  assert(HealthMonitor_Arm(&monitor, 1U, 0U));
  assert(HealthMonitor_Progress(&monitor, 0U, 1U));
  return 0;
}
