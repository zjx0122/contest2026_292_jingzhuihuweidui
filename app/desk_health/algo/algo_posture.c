/* algo_posture.c - 坐姿分类 + 预警状态机
 *
 * 逻辑规则：
 * - 离座<3分钟：暂停计时，返回后继续
 * - 离座>=3分钟：视为有效休息，计时清零
 * - 久坐超时后每15分钟重复提醒，最多3次
 * - 多异常优先级：久坐>前倾>驼背>环境
 */

#include <nuttx/config.h>
#include <string.h>
#include <time.h>
#include "desk_health.h"

/* 久坐计时状态 */

static uint32_t s_sit_accum_sec;       /* 累积在位秒数 */
static uint32_t s_leave_start_sec;     /* 离座开始时间 */
static bool     s_leave_timing;        /* 正在计时离座 */
static uint32_t s_sit_repeat_count;    /* 久坐重复提醒次数 */
static uint32_t s_last_alert_sec;      /* 上次久坐预警时间 */
static uint32_t s_posture_start_sec;   /* 当前不良坐姿开始时间 */
static enum posture_state_e s_prev_state = POSTURE_UNKNOWN;

void posture_init(void)
{
  s_sit_accum_sec = 0;
  s_leave_start_sec = 0;
  s_leave_timing = false;
  s_sit_repeat_count = 0;
  s_last_alert_sec = 0;
  s_posture_start_sec = 0;
  s_prev_state = POSTURE_UNKNOWN;
}

/* 获取当前时间秒数 */

static uint32_t now_sec(void)
{
  return (uint32_t)time(NULL);
}

/* 坐姿分类主函数 */

void posture_classify(struct desk_health_ctx_s *ctx)
{
  uint32_t now = now_sec();

  pthread_mutex_lock(&ctx->sensor_lock);
  fusion_update(&ctx->radar, &ctx->distance, &ctx->posture);
  pthread_mutex_unlock(&ctx->sensor_lock);

  /* 距离过近检测 */

  if (ctx->distance.data_valid &&
      ctx->distance.distance_mm > 0 &&
      ctx->distance.distance_mm < ctx->config.dist_threshold_cm * 10)
    {
      ctx->stats.distance_too_close++;
    }

  /* 更新久坐计时 */

  if (ctx->posture.state == POSTURE_LEFT_SEAT)
    {
      /* 离座处理 */

      if (!s_leave_timing)
        {
          s_leave_start_sec = now;
          s_leave_timing = true;
        }

      uint32_t leave_dur = now - s_leave_start_sec;

      if (leave_dur >= DEFAULT_LEAVE_RESET_MIN * 60)
        {
          /* 离座>=3分钟：有效休息，清零 */

          s_sit_accum_sec = 0;
          s_sit_repeat_count = 0;
          s_last_alert_sec = 0;
          s_leave_timing = false;
        }

      /* 离座期间不累积坐姿时间 */
    }
  else
    {
      /* 在位：恢复计时 */

      if (s_leave_timing)
        {
          /* 从离座返回，继续累积 */

          s_leave_timing = false;
        }

      s_sit_accum_sec++;
      ctx->stats.total_sit_cont_sec = s_sit_accum_sec;
    }

  /* 久坐超时判定 */

  uint16_t sit_threshold_sec = ctx->config.sit_threshold_min * 60;

  if (s_sit_accum_sec >= sit_threshold_sec &&
      ctx->config.sit_alert_enabled &&
      ctx->posture.state != POSTURE_LEFT_SEAT)
    {
      /* 首次超时或重复提醒 */

      if (s_last_alert_sec == 0 ||
          (now - s_last_alert_sec >= DEFAULT_SIT_REPEAT_MIN * 60 &&
           s_sit_repeat_count < DEFAULT_SIT_REPEAT_MAX))
        {
          ctx->posture.state = POSTURE_SIT_TOO_LONG;
          s_last_alert_sec = now;
          s_sit_repeat_count++;
        }
    }

  /* 更新不良坐姿持续时间 */

  if (ctx->posture.state == POSTURE_LEAN_FORWARD ||
      ctx->posture.state == POSTURE_HUNCHBACK)
    {
      if (s_prev_state != ctx->posture.state)
        {
          s_posture_start_sec = now;
        }

      ctx->posture.duration_sec = now - s_posture_start_sec;
    }
  else
    {
      s_posture_start_sec = now;
    }

  s_prev_state = ctx->posture.state;
}

/* 更新预警状态机 */

void posture_update_timers(struct desk_health_ctx_s *ctx)
{
  struct alert_state_s *a = &ctx->alert;
  uint32_t now = now_sec();
  enum posture_state_e ps = ctx->posture.state;

  /* 正常坐姿或离座 → 清除所有预警 */

  if (ps == POSTURE_NORMAL || ps == POSTURE_LEFT_SEAT)
    {
      a->level = ALERT_NONE;
      a->l1_start_sec = 0;
      a->l2_start_sec = 0;
      a->l3_start_sec = 0;
      a->l3_count = 0;
      a->popup_active = false;
      a->dismissed = false;
      ctx->voice_playing = false;
      return;
    }

  /* 用户手动关闭 → 保持当前级别但不升级 */

  if (a->dismissed)
    {
      return;
    }

  /* 优先级：久坐>前倾>驼背 */

  if (a->trigger != POSTURE_UNKNOWN &&
      ps != POSTURE_SIT_TOO_LONG &&
      a->trigger == POSTURE_SIT_TOO_LONG)
    {
      /* 久坐预警优先，不被其他覆盖 */

      return;
    }

  /* L1: 不良坐姿持续 >= 10秒 */

  if (a->l1_start_sec == 0)
    {
      a->l1_start_sec = now;
    }

  if (now - a->l1_start_sec >= ALERT_L1_DELAY_SEC &&
      a->level < ALERT_L1)
    {
      a->level = ALERT_L1;
      a->trigger = ps;
      ctx->stats.alert_count++;
    }

  /* L2: L1 持续 >= 15秒未纠正 */

  if (a->level == ALERT_L1 &&
      now - a->l1_start_sec >= ALERT_L1_DELAY_SEC + ALERT_L2_DELAY_SEC)
    {
      if (a->l2_start_sec == 0)
        {
          a->l2_start_sec = now;
        }

      a->level = ALERT_L2;
      a->popup_active = true;
    }

  /* L3: L2 持续 >= 60秒未纠正 */

  if (a->level == ALERT_L2 &&
      now - a->l2_start_sec >= ALERT_L3_DELAY_SEC)
    {
      if (a->l3_start_sec == 0)
        {
          a->l3_start_sec = now;
        }

      if (a->l3_count < ALERT_L3_REPEAT_MAX &&
          (a->l3_count == 0 ||
           now - a->l3_start_sec >= ALERT_L3_REPEAT_SEC * a->l3_count))
        {
          a->level = ALERT_L3;
          a->l3_count++;
        }
    }

  /* L2 弹窗自动消失 */

  if (a->popup_active && a->level == ALERT_L2 &&
      a->l2_start_sec > 0 &&
      now - a->l2_start_sec >= ALERT_L2_POPUP_SEC)
    {
      a->popup_active = false;
    }
}
