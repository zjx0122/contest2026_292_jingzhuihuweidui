/* task_esp32_comm.c - ESP32-S3 通信任务 */

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "desk_health.h"

#define ESP32_DEV "/dev/ttyS2"
#define RECV_BUF_SIZE 512

static int send_tts(int fd, const char *text)
{
  char buf[MQ_MSG_SIZE];
  snprintf(buf, sizeof(buf), "%s%s\n", PROTO_PREFIX_TTS, text);
  return write(fd, buf, strlen(buf));
}

static int send_app_data(int fd, struct desk_health_ctx_s *ctx)
{
  char json[MQ_MSG_SIZE];
  char buf[MQ_MSG_SIZE + 16];

  app_proto_build_json(ctx, json, sizeof(json));
  snprintf(buf, sizeof(buf), "%s%s\n", PROTO_PREFIX_APP, json);
  return write(fd, buf, strlen(buf));
}

static void handle_voice_cmd(struct desk_health_ctx_s *ctx,
                             int fd, const char *cmd)
{
  char reply[128];

  if (strstr(cmd, "温度") || strstr(cmd, "多少度"))
    {
      if (ctx->env.data_valid)
        snprintf(reply, sizeof(reply),
                 "现在温度%.1f度，湿度%.0f%%",
                 ctx->env.temperature, ctx->env.humidity);
      else
        snprintf(reply, sizeof(reply), "传感器数据异常");
      send_tts(fd, reply);
    }
  else if (strstr(cmd, "距离") || strstr(cmd, "多远"))
    {
      if (ctx->distance.data_valid)
        snprintf(reply, sizeof(reply),
                 "你离屏幕%d厘米",
                 ctx->distance.distance_mm / 10);
      else
        snprintf(reply, sizeof(reply), "测距传感器异常");
      send_tts(fd, reply);
    }
  else if (strstr(cmd, "坐了多久") || strstr(cmd, "久坐"))
    {
      uint32_t min = ctx->stats.total_sit_cont_sec / 60;
      snprintf(reply, sizeof(reply),
               "你已经连续坐了%d分钟", (int)min);
      send_tts(fd, reply);
    }
  else if (strstr(cmd, "知道了") || strstr(cmd, "好的"))
    {
      ctx->alert.dismissed = true;
      ctx->alert.popup_active = false;
    }
}

int task_esp32_main(int argc, char *argv[])
{
  struct desk_health_ctx_s *ctx;
  int fd;
  char recv_buf[RECV_BUF_SIZE];
  int recv_len = 0;
  uint32_t last_report = 0;

  ctx = (struct desk_health_ctx_s *)arg;

  fd = open(ESP32_DEV, O_RDWR | O_NONBLOCK);
  if (fd < 0)
    {
      printf("ESP32: 打开 %s 失败\n", ESP32_DEV);
      return NULL;
    }

  ctx->esp32_ready = true;

  while (ctx->running)
    {
      /* 接收 ESP32 数据 */

      int n = read(fd, recv_buf + recv_len,
                   RECV_BUF_SIZE - recv_len - 1);
      if (n > 0)
        {
          recv_len += n;
          recv_buf[recv_len] = '\0';

          /* 按行解析 */

          char *line = strtok(recv_buf, "\n");
          while (line)
            {
              if (strncmp(line, PROTO_PREFIX_VOICE,
                          strlen(PROTO_PREFIX_VOICE)) == 0)
                {
                  handle_voice_cmd(ctx, fd,
                                   line + strlen(PROTO_PREFIX_VOICE));
                }
              else if (strncmp(line, PROTO_PREFIX_APP,
                               strlen(PROTO_PREFIX_APP)) == 0)
                {
                  app_proto_parse_config(
                      line + strlen(PROTO_PREFIX_APP),
                      &ctx->config);
                }

              line = strtok(NULL, "\n");
            }

          recv_len = 0;
        }

      /* 定时上报 APP 数据 */

      uint32_t now = (uint32_t)time(NULL);
      if (ctx->app_connected && now - last_report >= 60)
        {
          send_app_data(fd, ctx);
          last_report = now;
        }

      /* TTS 播报预警 */

      if (ctx->alert.level == ALERT_L3 &&
          !ctx->voice_playing &&
          ctx->config.voice_alert_enabled)
        {
          send_tts(fd, "请注意坐姿，不要前倾");
          ctx->voice_playing = true;
        }

      usleep(100000);
    }

  close(fd);
  return NULL;
}
