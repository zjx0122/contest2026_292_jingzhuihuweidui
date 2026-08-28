/* app_protocol.c - APP JSON 协议构建/解析 */

#include <nuttx/config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "desk_health.h"

static const char *posture_str[] =
{
  "unknown", "normal", "lean_forward",
  "hunchback", "sit_too_long", "left_seat"
};

int app_proto_build_json(const struct desk_health_ctx_s *ctx,
                         char *buf, int buflen)
{
  pthread_mutex_lock(&ctx->sensor_lock);
  pthread_mutex_lock(&ctx->stats_lock);

  snprintf(buf, buflen,
    "{\"type\":\"data\""
    ",\"temp\":%.1f"
    ",\"humi\":%.0f"
    ",\"dist\":%d"
    ",\"sit\":%lu"
    ",\"posture\":\"%s\""
    ",\"present\":%s"
    ",\"bad_cnt\":%lu"
    ",\"env_cnt\":%lu}"
    ,
    ctx->env.temperature,
    ctx->env.humidity,
    ctx->distance.distance_mm / 10,
    (unsigned long)ctx->stats.total_sit_cont_sec,
    posture_str[ctx->posture.state],
    ctx->radar.target_present ? "true" : "false",
    (unsigned long)ctx->stats.bad_posture_count,
    (unsigned long)ctx->stats.env_alert_count);

  pthread_mutex_unlock(&ctx->stats_lock);
  pthread_mutex_unlock(&ctx->sensor_lock);
  return 0;
}

int app_proto_parse_config(const char *json,
                           struct system_config_s *config)
{
  /* 简易 JSON 解析：查找关键字段 */

  const char *p;

  p = strstr(json, "\"sit_min\"");
  if (p)
    {
      p = strchr(p, ':');
      if (p) config->sit_threshold_min = atoi(p + 1);
    }

  p = strstr(json, "\"dist_cm\"");
  if (p)
    {
      p = strchr(p, ':');
      if (p) config->dist_threshold_cm = atoi(p + 1);
    }

  return 0;
}
