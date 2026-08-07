/*
 ******************************************************************************
 * @file           : at_codec.c
 * @brief          : Common helpers for packing and unpacking AT text protocols
 ******************************************************************************
 */
#include "AT/at_codec.h"

#include <stdio.h>
#include <string.h>

static void AT_Clear(char *output, size_t output_size)
{
  if ((output != NULL) && (output_size > 0U)) {
    output[0] = '\0';
  }
}

AT_CodecResult AT_FormatV(char *output, size_t output_size, const char *format,
                          va_list args)
{
  int written;

  if ((output == NULL) || (output_size == 0U) || (format == NULL)) {
    AT_Clear(output, output_size);
    return AT_CODEC_INVALID_ARGUMENT;
  }
  written = vsnprintf(output, output_size, format, args);
  if (written < 0) {
    AT_Clear(output, output_size);
    return AT_CODEC_FORMAT_ERROR;
  }
  if ((size_t)written >= output_size) {
    AT_Clear(output, output_size);
    return AT_CODEC_BUFFER_TOO_SMALL;
  }
  return AT_CODEC_OK;
}

AT_CodecResult AT_Format(char *output, size_t output_size, const char *format,
                         ...)
{
  AT_CodecResult result;
  va_list args;

  va_start(args, format);
  result = AT_FormatV(output, output_size, format, args);
  va_end(args);
  return result;
}

AT_CodecResult AT_FinishPacket(char *packet, size_t packet_size,
                               size_t *length, AT_CodecResult result)
{
  if (length != NULL) {
    *length = 0U;
  }
  if (result != AT_CODEC_OK) {
    AT_Clear(packet, packet_size);
    return result;
  }
  if ((packet == NULL) || (packet_size == 0U)) {
    return AT_CODEC_INVALID_ARGUMENT;
  }
  if (length != NULL) {
    *length = strlen(packet);
  }
  return AT_CODEC_OK;
}

int AT_IsSingleLine(const char *text)
{
  if (text == NULL) {
    return 0;
  }
  return (strchr(text, '\r') == NULL) && (strchr(text, '\n') == NULL);
}

const char *AT_ReadLine(const char *cursor, AT_Line *line)
{
  const char *end;

  if ((cursor == NULL) || (line == NULL)) {
    return NULL;
  }
  end = cursor;
  while ((*end != '\0') && (*end != '\r') && (*end != '\n')) {
    ++end;
  }
  line->data = cursor;
  line->length = (size_t)(end - cursor);
  AT_TrimLine(line);
  while ((*end == '\r') || (*end == '\n')) {
    ++end;
  }
  return end;
}

void AT_TrimLine(AT_Line *line)
{
  if ((line == NULL) || (line->data == NULL)) {
    return;
  }
  while ((line->length > 0U) &&
         ((*line->data == ' ') || (*line->data == '\t'))) {
    ++line->data;
    --line->length;
  }
  while ((line->length > 0U) &&
         ((line->data[line->length - 1U] == ' ') ||
          (line->data[line->length - 1U] == '\t'))) {
    --line->length;
  }
}

int AT_LineEquals(const AT_Line *line, const char *token)
{
  size_t token_length;

  if ((line == NULL) || (token == NULL)) {
    return 0;
  }
  token_length = strlen(token);
  return (line->length == token_length) &&
         (strncmp(line->data, token, token_length) == 0);
}

int AT_LineStartsWith(const AT_Line *line, const char *prefix)
{
  size_t prefix_length;

  if ((line == NULL) || (prefix == NULL)) {
    return 0;
  }
  prefix_length = strlen(prefix);
  return (line->length >= prefix_length) &&
         (strncmp(line->data, prefix, prefix_length) == 0);
}

int AT_HasToken(const char *response, const char *token)
{
  return (response != NULL) && (token != NULL) &&
         (strstr(response, token) != NULL);
}

int AT_HasFinalResult(const char *response)
{
  const char *cursor = response;
  AT_Line line;

  while ((cursor != NULL) && (*cursor != '\0')) {
    cursor = AT_ReadLine(cursor, &line);
    if (AT_LineEquals(&line, "OK") || AT_LineEquals(&line, "ERROR") ||
        AT_LineStartsWith(&line, "+CME ERROR:") ||
        AT_LineStartsWith(&line, "+CMS ERROR:")) {
      return 1;
    }
  }
  return 0;
}

int AT_HasErrorResult(const char *response)
{
  const char *cursor = response;
  AT_Line line;

  while ((cursor != NULL) && (*cursor != '\0')) {
    cursor = AT_ReadLine(cursor, &line);
    if (AT_LineEquals(&line, "ERROR") ||
        AT_LineStartsWith(&line, "+CME ERROR:") ||
        AT_LineStartsWith(&line, "+CMS ERROR:")) {
      return 1;
    }
  }
  return 0;
}

void AT_CopyText(char *destination, size_t destination_size,
                 const char *source, size_t source_length,
                 uint8_t *truncated)
{
  size_t copy_length = source_length;

  if ((destination == NULL) || (destination_size == 0U)) {
    if (truncated != NULL) {
      *truncated = 1U;
    }
    return;
  }
  if (source == NULL) {
    destination[0] = '\0';
    return;
  }
  if (copy_length >= destination_size) {
    copy_length = destination_size - 1U;
    if (truncated != NULL) {
      *truncated = 1U;
    }
  }
  if (copy_length > 0U) {
    memcpy(destination, source, copy_length);
  }
  destination[copy_length] = '\0';
}

void AT_CopyString(char *destination, size_t destination_size,
                   const char *source, uint8_t *truncated)
{
  AT_CopyText(destination, destination_size, source,
              (source != NULL) ? strlen(source) : 0U, truncated);
}

void AT_AppendLine(char *destination, size_t destination_size,
                   const char *line, size_t line_length, uint8_t *truncated)
{
  size_t used;
  size_t available;

  if ((destination == NULL) || (destination_size == 0U) || (line == NULL) ||
      (line_length == 0U)) {
    return;
  }
  used = strlen(destination);
  if (used >= (destination_size - 1U)) {
    if (truncated != NULL) {
      *truncated = 1U;
    }
    return;
  }
  if (used > 0U) {
    destination[used++] = '\n';
    destination[used] = '\0';
  }
  available = destination_size - used - 1U;
  if (line_length > available) {
    line_length = available;
    if (truncated != NULL) {
      *truncated = 1U;
    }
  }
  if (line_length > 0U) {
    memcpy(&destination[used], line, line_length);
    destination[used + line_length] = '\0';
  }
}
