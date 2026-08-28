/* task_distance.c - VL53L0X 测距任务 */

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <nuttx/sensors/ioctl.h>
#include <time.h>
#include "desk_health.h"

#define DIST_DEV "/dev/vl53l0x"
#define DIST_HISTORY_LEN 5

void *task_distance_main(void *arg)
{
  struct desk_health_ctx_s *ctx;
  int fd;
  uint16_t raw_mm;
  float history[DIST_HISTORY_LEN];
  int hist_idx = 0;
  int fail_count = 0;

  ctx = (struct desk_health_ctx_s *)arg;
  memset(history, 0, sizeof(history));

  fd = open(DIST_DEV, O_RDONLY);
  if (fd < 0)
    {
      printf("测距: 打开 %s 失败\n", DIST_DEV);
      return NULL;
    }

  while (ctx->running)
    {
      /* 低功耗模式下关闭测距 */

      if (ctx->power_mode == POWER_DEEP_SLEEP)
        {
          usleep(1000000);
          continue;
        }

      pthread_mutex_lock(&ctx->i2c_lock);

      if (read(fd, &raw_mm, sizeof(raw_mm)) == sizeof(raw_mm))
        {
          float filtered = filter_moving_avg((float)raw_mm,
                                             history, DIST_HISTORY_LEN);

          pthread_mutex_lock(&ctx->sensor_lock);
          ctx->distance.distance_mm = (uint16_t)filtered;
          ctx->distance.data_valid = true;
          ctx->distance.timestamp = (uint32_t)time(NULL);
          pthread_mutex_unlock(&ctx->sensor_lock);

          fail_count = 0;
        }
      else
        {
          fail_count++;
          if (fail_count >= 5)
            {
              pthread_mutex_lock(&ctx->sensor_lock);
              ctx->distance.data_valid = false;
              pthread_mutex_unlock(&ctx->sensor_lock);
            }
        }

      pthread_mutex_unlock(&ctx->i2c_lock);
      usleep(DIST_POLL_MS * 1000);
    }

  close(fd);
  return NULL;
}
