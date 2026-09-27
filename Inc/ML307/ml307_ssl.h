/* ML307C bounded TLS configuration, certificate and module-clock helpers. */
#ifndef STM32TOOLS_ML307_SSL_H
#define STM32TOOLS_ML307_SSL_H

#include <stddef.h>
#include <stdint.h>

#include <ML307/ml307.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML307_SSL_CONTEXT_MAX 5U
#define ML307_SSL_CERT_NAME_MAX 64U
#define ML307_SSL_CERTIFICATE_MAX 8192U

typedef enum {
  ML307_SSL_OPTION_AUTH = 0,
  ML307_SSL_OPTION_VERSION,
  ML307_SSL_OPTION_IGNORE_TIMESTAMP,
  ML307_SSL_OPTION_IGNORE_VERIFICATION,
  ML307_SSL_OPTION_ENCODING,
  ML307_SSL_OPTION_CERTIFICATE
} ML307_SslOption;

typedef enum {
  ML307_SSL_AUTH_NONE = 0,
  ML307_SSL_AUTH_SERVER = 1,
  ML307_SSL_AUTH_MUTUAL = 2
} ML307_SslAuth;

typedef enum {
  ML307_SSL_VERSION_SSL3 = 0,
  ML307_SSL_VERSION_TLS10 = 1,
  ML307_SSL_VERSION_TLS11 = 2,
  ML307_SSL_VERSION_TLS12 = 3,
  ML307_SSL_VERSION_ALL = 255
} ML307_SslVersion;

typedef enum {
  ML307_SSL_ENCODING_ESCAPED = 2
} ML307_SslEncoding;

typedef enum {
  ML307_SSL_PARSE_INVALID = -1,
  ML307_SSL_PARSE_INCOMPLETE = 0,
  ML307_SSL_PARSE_COMPLETE = 1,
  ML307_SSL_PARSE_ERROR_RESPONSE = 2,
  ML307_SSL_PARSE_BUFFER_TOO_SMALL = 3,
  /** A complete +MATREADY line was observed in a proven control region. */
  ML307_SSL_PARSE_MODULE_RESET = 4
} ML307_SslParseResult;

typedef struct {
  ML307_SslOption option;
  uint8_t ssl_id;
  uint32_t value;
} ML307_SslOptionValue;

typedef struct {
  char server_ca[ML307_SSL_CERT_NAME_MAX + 1U];
  char client_certificate[ML307_SSL_CERT_NAME_MAX + 1U];
  char private_key[ML307_SSL_CERT_NAME_MAX + 1U];
} ML307_SslCertificateNames;

/** One certificate source and the exact slice to send after the '>' prompt. */
typedef struct {
  const char *name;
  const uint8_t *certificate;
  size_t certificate_length;
  size_t offset;
  size_t length;
} ML307_SslCertificateChunk;

/** Calendar time represented by AT+CCLK. timezone_quarters is UTC offset / 15m. */
typedef struct {
  uint16_t year; /* ML307C: 1970..2037 */
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  int8_t timezone_quarters; /* ML307C: -48..+56; rejects -01/+01 */
} ML307_ModuleClock;

typedef enum {
  ML307_SSL_ACTION_NONE = 0,
  ML307_SSL_ACTION_AT_COMMAND = 1,
  ML307_SSL_ACTION_RAW_PAYLOAD = 2
} ML307_SslActionKind;

typedef struct {
  ML307_SslActionKind kind;
  const uint8_t *data;
  size_t length;
} ML307_SslAction;

typedef enum {
  ML307_SSL_TRANSACTION_IDLE = 0,
  ML307_SSL_TRANSACTION_ACTIVE = 1,
  ML307_SSL_TRANSACTION_COMPLETE = 2,
  ML307_SSL_TRANSACTION_FAILED = 3
} ML307_SslTransactionStatus;

/** Bounded, allocation-free command sequencer. The product adapter owns the
 * UART, receive buffer and monotonic timeout. Certificate storage remains
 * caller-owned and must stay valid until completion/cancellation. */
typedef struct {
  uint8_t step;
  uint8_t option_index;
  uint8_t ssl_id;
  uint8_t mqtt_connect_id;
  uint8_t status;
  char ca_name[ML307_SSL_CERT_NAME_MAX + 1U];
  const uint8_t *certificate;
  size_t certificate_length;
  ML307_ModuleClock clock;
} ML307_SslTransaction;

/** Build one secure ML307C setting. Accepted values are deliberately limited
 * to auth=1, version=3, ignorestamp=0, ignoreverify=0 and encoding=2.
 */
ML307_Result ML307_SslBuildSetOption(char *output, size_t output_size,
                                    ML307_SslOption option, uint8_t ssl_id,
                                    uint32_t value);
ML307_Result ML307_SslBuildQueryOption(char *output, size_t output_size,
                                      ML307_SslOption option, uint8_t ssl_id);
ML307_SslParseResult ML307_SslParseOptionQuery(
    const uint8_t *response, size_t length, ML307_SslOption option,
    uint8_t ssl_id, ML307_SslOptionValue *value);

ML307_Result ML307_SslBuildSetCa(char *output, size_t output_size,
                                uint8_t ssl_id, const char *ca_name);
ML307_SslParseResult ML307_SslParseCertificateQuery(
    const uint8_t *response, size_t length, uint8_t ssl_id,
    ML307_SslCertificateNames *names);

ML307_Result ML307_SslBuildCertificateWrite(
    char *output, size_t output_size,
    const ML307_SslCertificateChunk *chunk);
ML307_Result ML307_SslGetCertificateWritePayload(
    const ML307_SslCertificateChunk *chunk, const uint8_t **payload,
    size_t *payload_length);
ML307_SslParseResult ML307_SslParseCertificateWritePrompt(
    const uint8_t *response, size_t length);
ML307_SslParseResult ML307_SslParseFinalResult(const uint8_t *response,
                                               size_t length);

ML307_Result ML307_SslBuildCertificateRead(char *output, size_t output_size,
                                           const char *name);
ML307_SslParseResult ML307_SslParseCertificateRead(
    const uint8_t *response, size_t length, uint8_t *certificate,
    size_t certificate_capacity, size_t *certificate_length);

ML307_Result ML307_SslBuildClockSet(char *output, size_t output_size,
                                   const ML307_ModuleClock *clock);
ML307_Result ML307_SslBuildClockQuery(char *output, size_t output_size);
ML307_SslParseResult ML307_SslParseClockQuery(const uint8_t *response,
                                              size_t length,
                                              ML307_ModuleClock *clock);

/** Convert trusted UTC seconds to an ML307C-representable UTC calendar. */
ML307_Result ML307_SslClockFromUnixUtc(uint32_t unix_time,
                                       ML307_ModuleClock *clock);

void ML307_SslTransactionInit(ML307_SslTransaction *transaction);
ML307_Result ML307_SslTransactionStartCaInstall(
    ML307_SslTransaction *transaction, const char *ca_name,
    const uint8_t *certificate, size_t certificate_length);
ML307_Result ML307_SslTransactionStartPrepare(
    ML307_SslTransaction *transaction, uint8_t ssl_id,
    uint8_t mqtt_connect_id, const char *ca_name,
    const ML307_ModuleClock *trusted_utc);
/** Obtain the next AT command or exact RAW certificate payload. */
ML307_Result ML307_SslTransactionGetAction(
    const ML307_SslTransaction *transaction, char *command,
    size_t command_capacity, ML307_SslAction *action);
/** Commit one successful adapter transmit; duplicate calls are rejected. */
ML307_Result ML307_SslTransactionActionSent(
    ML307_SslTransaction *transaction);
/** Inspect current buffered response without advancing transaction state. */
ML307_SslParseResult ML307_SslTransactionInspectResponse(
    const ML307_SslTransaction *transaction, const uint8_t *response,
    size_t response_length, uint8_t *certificate_scratch,
    size_t certificate_capacity);
/** Validate a complete response and advance to the next model-defined step. */
ML307_SslParseResult ML307_SslTransactionConsumeResponse(
    ML307_SslTransaction *transaction, const uint8_t *response,
    size_t response_length, uint8_t *certificate_scratch,
    size_t certificate_capacity);
void ML307_SslTransactionCancel(ML307_SslTransaction *transaction);
ML307_SslTransactionStatus ML307_SslTransactionGetStatus(
    const ML307_SslTransaction *transaction);

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_ML307_SSL_H */
