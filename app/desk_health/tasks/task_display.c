/* task_display.c - LCD UI 渲染任务 */

#include <nuttx/config.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/video/fb.h>
#include "desk_health.h"

#define FB_DEV "/dev/fb0"

static enum face_type_e get_face_for_posture(enum posture_state_e ps,
                                             enum alert_level_e al)
{
  if (al >= ALERT_L2) return FACE_ANGRY;
  if (ps == POSTURE_HUNCHBACK || ps == POSTURE_SIT_TOO_LONG)
    return FACE_TIRED;
  return FACE_HAPPY;
}

void *task_display_main(void *arg)
{
  struct desk_health_ctx_s *ctx;
  int fb_fd;
  uint32_t refresh_ms;

  ctx = (struct desk_health_ctx_s *)arg;

  fb_fd = open(FB_DEV, O_WRONLY);
  if (fb_fd < 0)
    {
      printf("显示: 打开 %s 失败\n", FB_DEV);
      return NULL;
    }

  ui_init();

  while (ctx->running)
    {
      /* 根据电源模式决定刷新频率 */

      if (ctx->power_mode == POWER_DEEP_SLEEP)
        {
          usleep(1000000);
          continue;
        }

      /* 更新桌宠表情 */

      enum face_type_e new_face =
          get_face_for_posture(ctx->posture.state, ctx->alert.level);

      if (ctx->voice_playing)
        new_face = FACE_TALKING;

      if (new_face != ctx->current_face)
        {
          ctx->current_face = new_face;
          ui_set_face(new_face);
        }

      /* 绘制主界面 */

      ui_draw_main_screen(ctx);

      /* 预警弹窗 */

      if (ctx->alert.popup_active)
        {
          ui_draw_alert_popup(ctx->alert.level, ctx->alert.trigger);
        }

      /* 触发 FB 更新 */

      ioctl(fb_fd, FBIO_UPDATE, 0);

      usleep(DISPLAY_REFRESH_MS * 1000);
    }

  close(fb_fd);
  return NULL;
}
