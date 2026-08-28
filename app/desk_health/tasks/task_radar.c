/* task_radar.c - LD2410B 雷达任务（优先级最高） */

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include "desk_health.h"

#define RADAR_DEV "/dev/ttyS1"
#define FRAME_HEADER 0xF4F3F2F1
#define FRAME_TAIL   0xF8F7F6F5

void *task_radar_main(void *arg)
{
  struct desk_health_ctx_s *ctx;
  int fd;
  uint8_t buf[64];
  int n;
  uint32_t last_no_target = 0;

  ctx = (struct desk_health_ctx_s *)arg;

  fd = open(RADAR_DEV, O_RDONLY);
  if (fd < 0)
    {
      printf("雷达: 打开 %s 失败\n", RADAR_DEV);
      return NULL;
    }

  while (ctx->running)
    {
      n = read(fd, buf, sizeof(buf));
      if (n < 23)
        {
          usleep(RADAR_POLL_MS * 1000);
          continue;
        }

      pthread_mutex_lock(&ctx->sensor_lock);

      /* 解析 LD2410B 帧：简化版，读取目标状态 */

      ctx->radar.target_state = buf[8];  /* 0=无, 1=静止, 2=运动 */
      ctx->radar.target_present = (ctx->radar.target_state != 0);
      ctx->radar.moving_distance = buf[10];
      ctx->radar.stationary_distance = buf[12];
      memcpy(ctx->radar.energy, &buf[14], 9);
      ctx->radar.data_valid = true;
      ctx->radar.timestamp = (uint32_t)time(NULL);

      pthread_mutex_unlock(&ctx->sensor_lock);

      /* 更新最后活动时间 */

      if (ctx->radar.target_present)
        {
          ctx->last_active_sec = (uint32_t)time(NULL);
        }
      else
        {
          if (last_no_target == 0)
            {
              last_no_target = (uint32_t)time(NULL);
            }
          else if ((uint32_t)time(NULL) - last_no_target >=
                   ctx->config.leave_detect_sec)
            {
              /* 离座确认 */
            }
        }

      usleep(RADAR_POLL_MS * 1000);
    }

  close(fd);
  return NULL;
}
