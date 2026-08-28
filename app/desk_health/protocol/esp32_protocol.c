/* esp32_protocol.c - ESP32 串口协议解析 */

#include <nuttx/config.h>
#include <string.h>
#include <stdio.h>
#include "desk_health.h"

int esp32_proto_send(const char *prefix, const char *data)
{
  /* 由 task_esp32_comm 调用实际发送 */
  return 0;
}

int esp32_proto_parse(char *buf, int len,
                      enum mq_msg_type_e *type, char *payload)
{
  if (len < 5) return -1;

  if (strncmp(buf, PROTO_PREFIX_TTS, strlen(PROTO_PREFIX_TTS)) == 0)
    {
      *type = MQ_MSG_TTS;
      strncpy(payload, buf + strlen(PROTO_PREFIX_TTS),
              MQ_MSG_SIZE - 1);
      return 0;
    }

  if (strncmp(buf, PROTO_PREFIX_VOICE, strlen(PROTO_PREFIX_VOICE)) == 0)
    {
      *type = MQ_MSG_VOICE_CMD;
      strncpy(payload, buf + strlen(PROTO_PREFIX_VOICE),
              MQ_MSG_SIZE - 1);
      return 0;
    }

  if (strncmp(buf, PROTO_PREFIX_APP, strlen(PROTO_PREFIX_APP)) == 0)
    {
      *type = MQ_MSG_APP_DATA;
      strncpy(payload, buf + strlen(PROTO_PREFIX_APP),
              MQ_MSG_SIZE - 1);
      return 0;
    }

  return -1;
}
