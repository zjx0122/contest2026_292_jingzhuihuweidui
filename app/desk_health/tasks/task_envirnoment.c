/* task_envirnoment.c - SHT30 温湿度任务 */

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <nuttx/sensors/ioctl.h>
#include <time.h>
#include "desk_health.h"

#define ENV_DEV "/dev/sht30"

static enum env_comfort_e classify_comfort(float temp, float humi)
{
  if (temp > ENV_HOT_TEMP) return ENV_COMFORT_HOT;
  if (temp > ENV_WARM_TEMP) return ENV_COMFORT_WARM;
  if (temp < ENV_COLD_TEMP) return ENV_COMFORT_COLD;
  if (temp < ENV_COOL_TEMP) return ENV_COMFORT_COOL;
  if (humi < ENV_DRY_HUMI) return ENV_COMFORT_DRY;
  if (humi > ENV_WET_HUMI) return ENV_COMFORT_WET;
  return ENV_COMFORT_GOOD;
}

void *task_environment_main(void *arg)
{
  struct desk_health_ctx_s *ctx;
  int fd;
  uint32_t poll_ms;
  int fail_count = 0;

  ctx = (struct desk_health_ctx_s *)arg;

  fd = open(ENV_DEV, O_RDONLY);
  if (fd < 0)
    {
      printf("温湿度: 打开 %s 失败\n", ENV_DEV);
      return NULL;
    }

  while (ctx->running)
    {
      poll_ms = (ctx->power_mode == POWER_DEEP_SLEEP)
                ? ENV_POLL_POWER_MS : ENV_POLL_NORMAL_MS;

      pthread_mutex_lock(&ctx->i2c_lock);

      float temp, humi;
      if (read(fd, &temp, sizeof(float)) == sizeof(float) &&
          read(fd, &humi, sizeof(float)) == sizeof(float))
        {
          pthread_mutex_lock(&ctx->sensor_lock);
          ctx->env.temperature = temp;
          ctx->env.humidity = humi;
          ctx->env.data_valid = true;
          ctx->env.timestamp = (uint32_t)time(NULL);
          pthread_mutex_unlock(&ctx->sensor_lock);

          ctx->env_comfort = classify_comfort(temp, humi);
          fail_count = 0;
        }
      else
        {
          fail_count++;
          if (fail_count >= 3)
            {
              ctx->env.data_valid = false;
            }
        }

      pthread_mutex_unlock(&ctx->i2c_lock);
      usleep(poll_ms * 1000);
    }

  close(fd);
  return NULL;
}
