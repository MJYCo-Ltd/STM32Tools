/*
 ******************************************************************************
 * @file           : ml307.h
 * @brief          : ML307 pack / unpack facade (no UART I/O)
 *
 * Application prepares content by type (+ strings). Driver packs a wire
 * string; application sends it. Responses are unpacked back into data.
 ******************************************************************************
 */
#ifndef STM32TOOLS_ML307_H
#define STM32TOOLS_ML307_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML307_DATA_NAME_SIZE 32U
#define ML307_DATA_TEXT_SIZE 512U
#define ML307_TOPIC_SIZE 128U
#define ML307_PAYLOAD_SIZE 256U

typedef enum {
  ML307_RESULT_OK = 0,
  ML307_RESULT_INVALID_ARGUMENT,
  ML307_RESULT_INVALID_VALUE,
  ML307_RESULT_BUFFER_TOO_SMALL,
  ML307_RESULT_NOT_FOUND,
  ML307_RESULT_ERROR_RESPONSE
} ML307_Result;

/** Logical request / response kinds (application-facing). */
typedef enum {
  ML307_TYPE_ATI = 0,
  ML307_TYPE_CEREG,
  ML307_TYPE_CIMI,
  ML307_TYPE_CESQ,
  ML307_TYPE_CGATT,
  ML307_TYPE_CCLK,
  ML307_TYPE_SLEEP,
  ML307_TYPE_MQTT_DISCONNECT,
  ML307_TYPE_MQTT_CLEAN,
  ML307_TYPE_MQTT_CONNECT,
  ML307_TYPE_MQTT_SUBSCRIBE,
  ML307_TYPE_MQTT_PUBLISH
} ML307_Type;

/** Content to pack. Unused fields may be NULL / 0. */
typedef struct {
  ML307_Type type;
  uint8_t id;       /**< MQTT connect id (0..5) */
  uint8_t flag;     /**< MQTT clean_session (0/1) */
  uint8_t qos;      /**< MQTT QoS */
  uint8_t retain;   /**< MQTT retain */
  uint16_t port;    /**< MQTT port */
  const char *host; /**< MQTT host / subscribe-publish topic */
  const char *client_id;
  const char *user;
  const char *password;
  const char *topic;   /**< explicit topic (sub/pub); if NULL, host is used */
  const char *message; /**< publish payload */
} ML307_Content;

/** Unpacked module data for the application. */
typedef struct {
  ML307_Type type;
  uint8_t ok;
  uint8_t error;
  int value; /**< CESQ RSRP, MQTT conn state, etc. (-1 if N/A) */
  uint8_t id;
  uint16_t message_id;
  uint8_t qos;
  char name[ML307_DATA_NAME_SIZE];
  char text[ML307_DATA_TEXT_SIZE];
  char topic[ML307_TOPIC_SIZE];
  char payload[ML307_PAYLOAD_SIZE];
} ML307_Data;

/** Short label for UI / logs, e.g. "CEREG", "MQTT_CONNECT". */
const char *ML307_TypeName(ML307_Type type);

/**
 * Pack content into a ready-to-send wire string.
 * On success, *length is the number of bytes to transmit (includes CRLF).
 */
ML307_Result ML307_Pack(const ML307_Content *content, char *packet,
                        size_t packet_size, size_t *length);

/**
 * Non-zero when the receive buffer has a final result for the given expect
 * type (OK/ERROR, or MQTT connection URC for MQTT_CONNECT).
 * id is used for MQTT connect matching (ignored for AT queries).
 */
int ML307_IsComplete(const char *packet, ML307_Type expect, uint8_t id);

/**
 * Unpack a module response for the request type that was sent.
 * Fills out->name / out->text (and typed fields when applicable).
 */
ML307_Result ML307_Unpack(const char *packet, ML307_Type expect,
                          ML307_Data *out);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_ML307_H */
