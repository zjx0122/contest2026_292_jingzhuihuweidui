/* algo_health.c - 健康数据统计 */

#include <nuttx/config.h>
#include <string.h>
#include <time.h>
#include "desk_health.h"

void health_stats_init(struct health_stats_s *stats)
{
  memset(stats, 0, sizeof(*stats));
}

void health_stats_update(struct desk_health_ctx_s *ctx)
{
  pthread_mutex_lock(&ctx->stats_lock);

  /* 在位时长累计 */

  if (ctx->posture.state != POSTURE_LEFT_SEAT &&
      ctx->posture.state != POSTURE_UNKNOWN)
    {
      ctx->stats.total_sit_sec++;
      ctx->stats.total_sit_cont_sec++;
    }
  else
    {
      /* 离座暂停连续计时，超过3分钟清零 */

      ctx->stats.total_sit_cont_sec = 0;
    }

  /* 不良坐姿次数 */

  if (ctx->posture.state == POSTURE_LEAN_FORWARD ||
      ctx->posture.state == POSTURE_HUNCHBACK)
    {
      ctx->stats.bad_posture_count++;
    }

  pthread_mutex_unlock(&ctx->stats_lock);
}

void health_stats_daily_reset(struct health_stats_s *stats)
{
  stats->total_sit_sec = 0;
  stats->total_sit_cont_sec = 0;
  stats->bad_posture_count = 0;
  stats->env_alert_count = 0;
  stats->alert_count = 0;
  stats->distance_too_close = 0;
}
