#include "servo_link.h"
#include "servo_test.h"
#include "serial_dma.h"
#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CACHE_SIZE 8U
typedef struct { uint32_t id; char request[64]; char response[128]; } CachedReply;
static CachedReply cache[CACHE_SIZE];
static uint8_t cache_head;
static uint32_t rx_errors;
static char payload[64], response[128], body[100];

static uint8_t Checksum(const char *data)
{
  uint8_t sum = 0U;
  while (*data) sum ^= (uint8_t)*data++;
  return sum;
}

static void SendBody(void)
{
  (void)snprintf(response, sizeof(response), "!%s*%02X\n", body, (unsigned)Checksum(body));
  (void)SerialDma_Write(response);
}

static void Error(uint32_t id, const char *reason)
{
  (void)snprintf(body, sizeof(body), "%lu,ERR,%s", (unsigned long)id, reason);
  SendBody();
}

static void Status(uint32_t id)
{
  ServoTestSnapshot state = ServoTest_GetSnapshot();
  (void)snprintf(body, sizeof(body), "%lu,OK,%s,%s,%lu,%lu,%lu",
                 (unsigned long)id, APP_FIRMWARE_VERSION, ServoTest_GetModeText(),
                 (unsigned long)state.errors, (unsigned long)state.commands,
                 (unsigned long)state.presses);
  SendBody();
}

static uint8_t Number(const char *text, int32_t minimum, int32_t maximum, int32_t *out)
{
  char *end;
  long value;
  size_t n;
  if (!text || !*text) return 0U;
  n = (text[0] == '-') ? 1U : 0U;
  if (!text[n] || strspn(text+n, "0123456789") != strlen(text+n) || strlen(text) > 8U) return 0U;
  value = strtol(text, &end, 10);
  if (*end || value < minimum || value > maximum) return 0U;
  *out = (int32_t)value;
  return 1U;
}

static void HandleLine(const char *line)
{
  char *star, *end, *fields[5], *token;
  uint8_t count = 0U, i, ok, motion;
  int32_t id, first = 0, second = 0;
  unsigned long checksum;
  if (line[0] != '@' || strlen(line) >= sizeof(payload)) return;
  (void)strcpy(payload, line+1);
  star = strchr(payload, '*');
  if (!star || strlen(star+1) != 2U || strspn(star+1,"0123456789abcdefABCDEF") != 2U) return;
  checksum = strtoul(star+1, &end, 16);
  *star = '\0';
  if (*end || Checksum(payload) != checksum || strstr(payload, ",,") || !*payload ||
      payload[strlen(payload)-1U] == ',') return;
  token = strtok(payload, ",");
  while (token && count < 5U) { fields[count++] = token; token = strtok(NULL, ","); }
  if (token || count < 2U || !Number(fields[0], 1, 65535, &id)) return;
  if (strlen(fields[1]) != 1U) { Error((uint32_t)id, "SYNTAX"); return; }
  motion = strchr("MCA", fields[1][0]) != NULL;
  if (motion)
  {
    for (i = 0U; i < CACHE_SIZE; ++i)
    {
      if (cache[i].id != (uint32_t)id) continue;
      if (!strcmp(cache[i].request, line)) (void)SerialDma_Write(cache[i].response);
      else Error((uint32_t)id, "ID_COLLISION");
      return;
    }
    if (count != 4U || !Number(fields[2], -7200, 7200, &first) || !first ||
        !Number(fields[3], 100, 1500, &second)) { Error((uint32_t)id, "RANGE"); return; }
    if (fields[1][0] == 'C' && first != 1 && first != -1) { Error((uint32_t)id, "RANGE"); return; }
  }
  else if (count != 2U) { Error((uint32_t)id, "SYNTAX"); return; }
  ok = 1U;
  switch (fields[1][0])
  {
    case 'V': break;
    case 'K': ServoTest_KeepAlive(); break;
    case 'S': ok = ServoTest_Stop(); break;
    case 'M': ok = ServoTest_Move((int16_t)first, (uint16_t)second); break;
    case 'A': ok = ServoTest_Automatic((int16_t)first, (uint16_t)second); break;
    case 'C': ok = ServoTest_Continuous((int8_t)first, (uint16_t)second); break;
    default: Error((uint32_t)id, "UNKNOWN"); return;
  }
  if (ok) Status((uint32_t)id);
  else Error((uint32_t)id, "SERVO_LINK");
  if (!motion) return;
  cache[cache_head].id = (uint32_t)id;
  (void)strcpy(cache[cache_head].request, line);
  (void)strcpy(cache[cache_head].response, response);
  cache_head = (cache_head + 1U) % CACHE_SIZE;
}

void ServoLink_Init(void)
{
  memset(cache, 0, sizeof(cache));
  cache_head = 0U;
  (void)SerialDma_InitBluetooth(HandleLine);
  rx_errors = SerialDma_GetRxErrors();
}

void ServoLink_Process(void)
{
  if (SerialDma_GetRxErrors() != rx_errors)
  {
    rx_errors = SerialDma_GetRxErrors();
    (void)ServoTest_Stop();
  }
  SerialDma_Process();
}
