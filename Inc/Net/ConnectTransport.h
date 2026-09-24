#ifndef STM32TOOLS_NET_CONNECT_TRANSPORT_H
#define STM32TOOLS_NET_CONNECT_TRANSPORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONNECT_DOWNLOAD_DATA_SIZE 256U
#define CONNECT_DOWNLOAD_URL_SIZE 192U

typedef struct {
  uint32_t content_length;
  /* Durable offset already committed by the product. A replacement transport
   * resumes here instead of restarting the higher-level download workflow. */
  uint32_t offset;
  char url[CONNECT_DOWNLOAD_URL_SIZE];
} ConnectDownloadRequest;

typedef enum {
  CONNECT_DOWNLOAD_EVENT_NONE = 0,
  CONNECT_DOWNLOAD_EVENT_DATA,
  CONNECT_DOWNLOAD_EVENT_COMPLETE,
  CONNECT_DOWNLOAD_EVENT_FAILED
} ConnectDownloadEventType;

typedef struct {
  ConnectDownloadEventType type;
  uint32_t offset;
  uint32_t content_length;
  uint32_t error_code;
  uint16_t data_length;
  /* 1 = current transfer ownership is released and its command channel is
   * synchronized for reuse. 0 = ownership/release is unknown; generic routers
   * must not fail over to another operation on this same adapter yet. */
  uint8_t transport_released;
  uint8_t data[CONNECT_DOWNLOAD_DATA_SIZE];
} ConnectDownloadEvent;

typedef uint8_t (*ConnectDownloadStart)(const ConnectDownloadRequest *request);
typedef uint8_t (*ConnectDownloadGetEvent)(ConnectDownloadEvent *event);
typedef uint8_t (*ConnectDownloadAcknowledge)(uint8_t accepted);

typedef struct {
  ConnectDownloadStart start;
  ConnectDownloadGetEvent get_event;
  ConnectDownloadAcknowledge acknowledge;
} ConnectDownloadOps;

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_NET_CONNECT_TRANSPORT_H */
