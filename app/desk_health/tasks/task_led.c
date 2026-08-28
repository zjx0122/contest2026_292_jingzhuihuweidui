/* task_led.c - LED 状态指示灯控制
 *
 * LED1(PD0) 在位：有人常亮，无人熄灭，坐姿预警时2Hz闪烁
 * LED2(PD1) 告警：正常熄灭，L1预警1Hz慢闪，L2/L3预警4Hz快闪
 * LED3(PE6) APP：未连接熄灭，连接常亮，传输中闪烁
 */

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <nuttx/ioexpander/gpio.h>
#include "desk_health.h"

#define LED1_DEV "/dev/gpio_led1"  /* PD0 在位 */
#define LED2_DEV "/dev/gpio_led2"  /* PD1 告警 */
#define LED3_DEV "/dev/gpio_led3"  /* PE6 APP */

#define LED_TICK_MS  125  /* 8Hz 基础节拍 */

static int led_write(int fd, bool on)
{
  uint8_t val = on ? 1 : 0;
  return write(fd, &val, 1);
}

void *task_led_main(void *arg)
{
  struct desk_health_ctx_s *ctx;
  int fd1, fd2, fd3;
  uint32_t tick = 0;

  ctx = (struct desk_health_ctx_s *)arg;

  fd1 = open(LED1_DEV, O_WRONLY);
  fd2 = open(LED2_DEV, O_WRONLY);
  fd3 = open(LED3_DEV, O_WRONLY);

  while (ctx->running)
    {
      bool present = (ctx->posture.state != POSTURE_LEFT_SEAT &&
                      ctx->posture.state != POSTURE_UNKNOWN);

      /* LED1 在位指示 */

      if (!present)
        {
          led_write(fd1, false);
        }
      else if (ctx->alert.level >= ALERT_L1)
        {
          /* 预警时 2Hz 闪烁 (每4tick翻转) */

          led_write(fd1, (tick % 4) < 2);
        }
      else
        {
          led_write(fd1, true);
        }

      /* LED2 告警指示 */

      if (ctx->alert.level == ALERT_L1)
        {
          /* 1Hz 慢闪 (每8tick翻转) */

          led_write(fd2, (tick % 8) < 4);
        }
      else if (ctx->alert.level >= ALERT_L2)
        {
          /* 4Hz 快闪 (每2tick翻转) */

          led_write(fd2, (tick % 2) == 0);
        }
      else
        {
          led_write(fd2, false);
        }

      /* LED3 APP 连接指示 */

      if (!ctx->app_connected)
        {
          led_write(fd3, false);
        }
      else if (ctx->voice_playing)
        {
          led_write(fd3, (tick % 2) == 0);
        }
      else
        {
          led_write(fd3, true);
        }

      tick++;
      usleep(LED_TICK_MS * 1000);
    }

  if (fd1 >= 0) close(fd1);
  if (fd2 >= 0) close(fd2);
  if (fd3 >= 0) close(fd3);
  return NULL;
}
