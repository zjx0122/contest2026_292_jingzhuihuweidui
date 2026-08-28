/* desk_health_main.c - 端侧AI桌面健康监测终端 主入口
 *
 * 任务优先级：
 * 1. AI坐姿推理 (主循环)
 * 2. 雷达+测距采集
 * 3. 环境传感采集
 * 4. 语音/APP交互
 * 5. LCD UI渲染
 * 6. LED指示灯
 * 7. 按键处理
 * 8. 数据持久化
 */

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "desk_health.h"

/* 全局上下文，所有任务共享 */

static struct desk_health_ctx_s g_ctx;

static void init_context(struct desk_health_ctx_s *ctx)
{
  memset(ctx, 0, sizeof(*ctx));

  ctx->config.sit_threshold_min = DEFAULT_SIT_THRESHOLD_MIN;
  ctx->config.dist_threshold_cm = DEFAULT_DIST_THRESHOLD_CM;
  ctx->config.lean_detect_sec = DEFAULT_LEAN_DETECT_SEC;
  ctx->config.leave_detect_sec = DEFAULT_LEAVE_DETECT_SEC;
  ctx->config.sit_alert_enabled = true;
  ctx->config.voice_alert_enabled = true;

  pthread_mutex_init(&ctx->sensor_lock, NULL);
  pthread_mutex_init(&ctx->config_lock, NULL);
  pthread_mutex_init(&ctx->stats_lock, NULL);
  pthread_mutex_init(&ctx->i2c_lock, NULL);
  pthread_mutex_init(&ctx->esp32_lock, NULL);

  filter_init();
  fusion_init();
  posture_init();
  health_stats_init(&ctx->stats);

  ctx->posture.state = POSTURE_UNKNOWN;
  ctx->env_comfort = ENV_COMFORT_GOOD;
  ctx->alert.level = ALERT_NONE;
  ctx->power_mode = POWER_NORMAL;
  ctx->current_face = FACE_HAPPY;
  ctx->running = true;
}

/* 启动一个 pthread 任务，直接传递 ctx 指针（无竞态） */

static int start_task(struct desk_health_ctx_s *ctx,
                      const char *name, int prio, int stack,
                      void *(*entry)(void *))
{
  pthread_t tid;
  pthread_attr_t attr;
  struct sched_param param;

  pthread_attr_init(&attr);
  param.sched_priority = prio;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, stack);

  if (pthread_create(&tid, &attr, entry, ctx) != 0)
    {
      printf("创建任务 %s 失败\n", name);
      return -1;
    }

  pthread_detach(tid);
  printf("任务 %s 已启动 (优先级 %d)\n", name, prio);
  return 0;
}

int main(int argc, char *argv[])
{
  printf("=== 端侧AI桌面健康监测终端 ===\n");
  printf("基于 OpenVela RTOS\n\n");

  init_context(&g_ctx);

  /* 启动传感器采集任务 */

  start_task(&g_ctx, "radar", TASK_PRIO_POSTURE,
             TASK_STACK_POSTURE, task_radar_main);

  start_task(&g_ctx, "distance", TASK_PRIO_POSTURE,
             TASK_STACK_POSTURE, task_distance_main);

  /* 启动环境监测任务 */

  start_task(&g_ctx, "environment", TASK_PRIO_ENVIRONMENT,
             TASK_STACK_ENVIRONMENT, task_environment_main);

  /* 启动通信任务 */

  start_task(&g_ctx, "esp32", TASK_PRIO_ESP32,
             TASK_STACK_ESP32, task_esp32_main);

  /* 启动显示任务 */

  start_task(&g_ctx, "display", TASK_PRIO_DISPLAY,
             TASK_STACK_DISPLAY, task_display_main);

  /* 启动 LED 任务 */

  start_task(&g_ctx, "led", TASK_PRIO_DISPLAY,
             1024, task_led_main);

  /* 启动按键任务 */

  start_task(&g_ctx, "button", TASK_PRIO_ESP32,
             1024, task_button_main);

  /* 启动存储任务 */

  start_task(&g_ctx, "storage", TASK_PRIO_STORAGE,
             TASK_STACK_STORAGE, task_storage_main);

  printf("所有任务已启动\n\n");

  /* 主循环：坐姿推理 + 预警状态机 + 统计更新 */

  while (g_ctx.running)
    {
      posture_classify(&g_ctx);
      posture_update_timers(&g_ctx);
      health_stats_update(&g_ctx);
      sleep(1);
    }

  return 0;
}
