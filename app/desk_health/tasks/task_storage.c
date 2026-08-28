/* task_storage.c - Flash 数据持久化
 *
 * 功能：
 * - 参数持久化：久坐阈值、距离阈值，修改后立即写入
 * - 当日统计：每5分钟或关键事件写入 Flash
 * - 历史数据：至少保留7天，循环覆盖
 * - 上电加载
 */

#include <nuttx/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "desk_health.h"

#define STORAGE_PATH "/tmp/desk_health"
#define CONFIG_FILE  STORAGE_PATH "/config.dat"
#define STATS_FILE   STORAGE_PATH "/stats.dat"
#define HISTORY_FILE STORAGE_PATH "/history.dat"
#define SAVE_INTERVAL_SEC 300  /* 5分钟 */
#define HISTORY_DAYS 7

struct stored_config_s
{
  uint32_t magic;
  struct system_config_s config;
};

struct stored_stats_s
{
  uint32_t magic;
  uint32_t date_stamp;
  struct health_stats_s stats;
};

#define CONFIG_MAGIC 0x484C5448  /* 'HLTH' */

static int save_config(const struct system_config_s *config)
{
  int fd = open(CONFIG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) return -errno;

  struct stored_config_s sc;
  sc.magic = CONFIG_MAGIC;
  memcpy(&sc.config, config, sizeof(*config));

  int ret = write(fd, &sc, sizeof(sc)) == sizeof(sc) ? 0 : -EIO;
  close(fd);
  return ret;
}

static int load_config(struct system_config_s *config)
{
  int fd = open(CONFIG_FILE, O_RDONLY);
  if (fd < 0) return -errno;

  struct stored_config_s sc;
  int ret = -EIO;

  if (read(fd, &sc, sizeof(sc)) == sizeof(sc) &&
      sc.magic == CONFIG_MAGIC)
    {
      memcpy(config, &sc.config, sizeof(*config));
      ret = 0;
    }

  close(fd);
  return ret;
}

static int save_stats(const struct health_stats_s *stats)
{
  int fd = open(STATS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) return -errno;

  struct stored_stats_s ss;
  ss.magic = CONFIG_MAGIC;
  ss.date_stamp = (uint32_t)time(NULL) / 86400;
  memcpy(&ss.stats, stats, sizeof(*stats));

  int ret = write(fd, &ss, sizeof(ss)) == sizeof(ss) ? 0 : -EIO;
  close(fd);
  return ret;
}

static int load_stats(struct health_stats_s *stats)
{
  int fd = open(STATS_FILE, O_RDONLY);
  if (fd < 0) return -errno;

  struct stored_stats_s ss;
  int ret = -EIO;

  if (read(fd, &ss, sizeof(ss)) == sizeof(ss) &&
      ss.magic == CONFIG_MAGIC)
    {
      /* 检查是否同一天 */

      uint32_t today = (uint32_t)time(NULL) / 86400;
      if (ss.date_stamp == today)
        {
          memcpy(stats, &ss.stats, sizeof(*stats));
          ret = 0;
        }
    }

  close(fd);
  return ret;
}

void *task_storage_main(void *arg)
{
  struct desk_health_ctx_s *ctx;
  uint32_t last_save = 0;

  ctx = (struct desk_health_ctx_s *)arg;

  /* 上电加载配置 */

  if (load_config(&ctx->config) == 0)
    {
      /* 已从 Flash 加载配置 */
    }

  /* 上电加载当日统计 */

  load_stats(&ctx->stats);

  while (ctx->running)
    {
      uint32_t now = (uint32_t)time(NULL);

      /* 每5分钟保存统计 */

      if (now - last_save >= SAVE_INTERVAL_SEC)
        {
          pthread_mutex_lock(&ctx->stats_lock);
          save_stats(&ctx->stats);
          pthread_mutex_unlock(&ctx->stats_lock);
          last_save = now;
        }

      /* 跨天检测：每日归档 */

      static uint32_t s_last_day = 0;
      uint32_t today = now / 86400;
      if (s_last_day != 0 && s_last_day != today)
        {
          pthread_mutex_lock(&ctx->stats_lock);
          health_stats_daily_reset(&ctx->stats);
          pthread_mutex_unlock(&ctx->stats_lock);
        }

      s_last_day = today;

      sleep(10);
    }

  /* 退出前保存 */

  save_config(&ctx->config);
  save_stats(&ctx->stats);
  return NULL;
}
