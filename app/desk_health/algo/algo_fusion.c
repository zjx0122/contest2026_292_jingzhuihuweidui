/* algo_fusion.c - 双传感器融合：雷达+测距 */

#include <nuttx/config.h>
#include <string.h>
#include "desk_health.h"

static uint32_t s_bad_start_sec;
static enum posture_state_e s_last_state = POSTURE_UNKNOWN;

void fusion_init(void)
{
  s_bad_start_sec = 0;
  s_last_state = POSTURE_UNKNOWN;
}

void fusion_update(const struct radar_data_s *radar,
                   const struct distance_data_s *dist,
                   struct posture_result_s *result)
{
  uint32_t now = (uint32_t)time(NULL);

  /* 离座判定：雷达无目标 */

  if (!radar->target_present || radar->target_state == 0)
    {
      if (s_last_state != POSTURE_LEFT_SEAT)
        {
          s_bad_start_sec = now;
          s_last_state = POSTURE_LEFT_SEAT;
        }

      result->state = POSTURE_LEFT_SEAT;
      result->confidence = radar->data_valid ? 90 : 50;
      result->duration_sec = now - s_bad_start_sec;
      return;
    }

  /* 前倾过近：雷达前倾特征 + 测距 < 阈值 */

  if (radar->target_state == 2 && dist->data_valid &&
      dist->distance_mm > 0 && dist->distance_mm < 350)
    {
      if (s_last_state != POSTURE_LEAN_FORWARD)
        {
          s_bad_start_sec = now;
          s_last_state = POSTURE_LEAN_FORWARD;
        }

      result->state = POSTURE_LEAN_FORWARD;
      result->confidence = 85;
      result->duration_sec = now - s_bad_start_sec;
      return;
    }

  /* 低头驼背：雷达静止+微动特征 */

  if (radar->stationary_distance > 0 && radar->stationary_distance < 80)
    {
      if (s_last_state != POSTURE_HUNCHBACK)
        {
          s_bad_start_sec = now;
          s_last_state = POSTURE_HUNCHBACK;
        }

      result->state = POSTURE_HUNCHBACK;
      result->confidence = 75;
      result->duration_sec = now - s_bad_start_sec;
      return;
    }

  /* 正常坐姿 */

  if (s_last_state != POSTURE_NORMAL)
    {
      s_bad_start_sec = now;
      s_last_state = POSTURE_NORMAL;
    }

  result->state = POSTURE_NORMAL;
  result->confidence = 80;
  result->duration_sec = now - s_bad_start_sec;
}
