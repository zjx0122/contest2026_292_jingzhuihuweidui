/* task_button.c - PA0 按键处理
 *
 * 短按(<1秒)：关闭预警弹窗 / 停止语音播报 / 低功耗唤醒
 * 长按(>=3秒)：重置当日数据
 */

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <nuttx/ioexpander/gpio.h>
#include <time.h>
#include "desk_health.h"

#define BTN_DEV "/dev/gpio_btn0"
#define LONG_PRESS_SEC 3

void *task_button_main(void *arg)
{
  struct desk_health_ctx_s *ctx;
  int fd;
  uint8_t val;
  uint32_t press_start = 0;
  bool pressed = false;

  ctx = (struct desk_health_ctx_s *)arg;

  fd = open(BTN_DEV, O_RDONLY);
  if (fd < 0)
    {
      printf("按键: 打开 %s 失败\n", BTN_DEV);
      return NULL;
    }

  while (ctx->running)
    {
      if (read(fd, &val, 1) == 1)
        {
          uint32_t now = (uint32_t)time(NULL);

          if (val == 0 && !pressed)
            {
              /* 按下 */

              pressed = true;
              press_start = now;
            }
          else if (val == 1 && pressed)
            {
              /* 松开 */

              pressed = false;
              uint32_t dur = now - press_start;

              if (dur < LONG_PRESS_SEC)
                {
                  /* 短按：关闭弹窗/停止语音 */

                  ctx->alert.dismissed = true;
                  ctx->alert.popup_active = false;
                  ctx->voice_playing = false;

                  /* 低功耗唤醒 */

                  if (ctx->power_mode != POWER_NORMAL)
                    {
                      ctx->power_mode = POWER_NORMAL;
                      ctx->last_active_sec = now;
                    }
                }
              else
                {
                  /* 长按：重置当日数据 */

                  pthread_mutex_lock(&ctx->stats_lock);
                  health_stats_daily_reset(&ctx->stats);
                  pthread_mutex_unlock(&ctx->stats_lock);
                }
            }
        }

      usleep(50000);  /* 50ms 轮询 */
    }

  close(fd);
  return NULL;
}
