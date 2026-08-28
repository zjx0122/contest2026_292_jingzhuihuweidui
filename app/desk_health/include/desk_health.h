/****************************************************************************
 * apps/examples/desk_health/include/desk_health.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.  See the License for the specific language governing
 * permissions and limitations under the License.
 *
 ****************************************************************************/

#ifndef __APPS_EXAMPLES_DESK_HEALTH_INCLUDE_DESK_HEALTH_H
#define __APPS_EXAMPLES_DESK_HEALTH_INCLUDE_DESK_HEALTH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <mqueue.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* 任务优先级 */

#define TASK_PRIO_POSTURE     100
#define TASK_PRIO_ENVIRONMENT 110
#define TASK_PRIO_ESP32       120
#define TASK_PRIO_DISPLAY     130
#define TASK_PRIO_STORAGE     150

/* 任务栈大小 */

#define TASK_STACK_POSTURE     2048
#define TASK_STACK_ENVIRONMENT 2048
#define TASK_STACK_ESP32       4096
#define TASK_STACK_DISPLAY     4096
#define TASK_STACK_STORAGE     2048

/* 默认阈值 */

#define DEFAULT_SIT_THRESHOLD_MIN    45
#define DEFAULT_DIST_THRESHOLD_CM    35
#define DEFAULT_LEAN_DETECT_SEC      10
#define DEFAULT_LEAVE_DETECT_SEC     15
#define DEFAULT_LEAVE_RESET_MIN      3
#define DEFAULT_SIT_REPEAT_MIN       15
#define DEFAULT_SIT_REPEAT_MAX       3

/* 预警时间阈值（秒） */

#define ALERT_L1_DELAY_SEC       10
#define ALERT_L2_DELAY_SEC       15
#define ALERT_L3_DELAY_SEC       60
#define ALERT_L2_POPUP_SEC        5
#define ALERT_L3_REPEAT_SEC      30
#define ALERT_L3_REPEAT_MAX       2

/* 环境舒适度阈值 */

#define ENV_COMFORT_TEMP_LOW     18.0f
#define ENV_COMFORT_TEMP_HIGH    28.0f
#define ENV_WARM_TEMP            32.0f
#define ENV_HOT_TEMP             35.0f
#define ENV_COOL_TEMP            10.0f
#define ENV_COLD_TEMP             5.0f
#define ENV_COMFORT_HUMI_LOW     40.0f
#define ENV_COMFORT_HUMI_HIGH    60.0f
#define ENV_DRY_HUMI             20.0f
#define ENV_WET_HUMI             80.0f

/* 低功耗时间阈值（秒） */

#define POWERDOWN_SCREEN_SEC     30
#define POWERDOWN_DEEP_SEC      300
#define POWERDOWN_WAKE_SEC        5

/* 采集周期（毫秒） */

#define RADAR_POLL_MS            100
#define DIST_POLL_MS             200
#define ENV_POLL_NORMAL_MS     30000
#define ENV_POLL_POWER_MS     300000
#define DISPLAY_REFRESH_MS       50
#define APP_REPORT_MS          60000

/* 通信协议前缀 */

#define PROTO_PREFIX_TTS         "TTS:"
#define PROTO_PREFIX_VOICE       "VOICE:"
#define PROTO_PREFIX_APP         "APP:"

/* 消息队列 */

#define MQ_MAX_MSGS              8
#define MQ_MSG_SIZE            256

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* 坐姿状态枚举 */

enum posture_state_e
{
  POSTURE_UNKNOWN = 0,
  POSTURE_NORMAL,
  POSTURE_LEAN_FORWARD,
  POSTURE_HUNCHBACK,
  POSTURE_SIT_TOO_LONG,
  POSTURE_LEFT_SEAT
};

/* 预警级别 */

enum alert_level_e
{
  ALERT_NONE = 0,
  ALERT_L1,
  ALERT_L2,
  ALERT_L3
};

/* 环境舒适度 */

enum env_comfort_e
{
  ENV_COMFORT_GOOD = 0,
  ENV_COMFORT_WARM,
  ENV_COMFORT_HOT,
  ENV_COMFORT_COOL,
  ENV_COMFORT_COLD,
  ENV_COMFORT_DRY,
  ENV_COMFORT_WET
};

/* LCD 桌宠表情 */

enum face_type_e
{
  FACE_HAPPY = 0,
  FACE_ANGRY,
  FACE_TIRED,
  FACE_THINKING,
  FACE_TALKING
};

/* 电源模式 */

enum power_mode_e
{
  POWER_NORMAL = 0,
  POWER_STANDBY,
  POWER_DEEP_SLEEP
};

/* LD2410B 雷达数据 */

struct radar_data_s
{
  bool     target_present;
  uint8_t  target_state;
  uint8_t  moving_distance;
  uint8_t  stationary_distance;
  uint8_t  energy[9];
  bool     data_valid;
  uint32_t timestamp;
};

/* VL53L0X 测距数据 */

struct distance_data_s
{
  uint16_t distance_mm;
  bool     data_valid;
  uint32_t timestamp;
};

/* SHT30 温湿度数据 */

struct env_data_s
{
  float    temperature;
  float    humidity;
  bool     data_valid;
  uint32_t timestamp;
};

/* 坐姿分析结果 */

struct posture_result_s
{
  enum posture_state_e state;
  uint8_t              confidence;
  uint32_t             duration_sec;
};

/* 预警状态 */

struct alert_state_s
{
  enum alert_level_e   level;
  enum posture_state_e trigger;
  uint32_t             l1_start_sec;
  uint32_t             l2_start_sec;
  uint32_t             l3_start_sec;
  uint32_t             l3_count;
  bool                 popup_active;
  bool                 dismissed;
};

/* 每日健康统计 */

struct health_stats_s
{
  uint32_t total_sit_sec;
  uint32_t total_sit_cont_sec;
  uint32_t bad_posture_count;
  uint32_t env_alert_count;
  uint32_t alert_count;
  uint32_t distance_too_close;
};

/* 可配置参数 */

struct system_config_s
{
  uint16_t sit_threshold_min;
  uint16_t dist_threshold_cm;
  uint16_t lean_detect_sec;
  uint16_t leave_detect_sec;
  bool     sit_alert_enabled;
  bool     voice_alert_enabled;
};

/* 全局共享状态 */

struct desk_health_ctx_s
{
  struct radar_data_s      radar;
  struct distance_data_s   distance;
  struct env_data_s        env;
  struct posture_result_s  posture;
  enum env_comfort_e       env_comfort;
  struct alert_state_s     alert;
  struct health_stats_s    stats;
  struct system_config_s   config;
  enum power_mode_e        power_mode;
  uint32_t                 last_active_sec;
  bool                     esp32_ready;
  bool                     app_connected;
  enum face_type_e         current_face;
  bool                     voice_playing;
  pthread_mutex_t          sensor_lock;
  pthread_mutex_t          config_lock;
  pthread_mutex_t          stats_lock;
  pthread_mutex_t          i2c_lock;
  pthread_mutex_t          esp32_lock;
  mqd_t                    mq_alert;
  mqd_t                    mq_esp32_tx;
  mqd_t                    mq_esp32_rx;
  volatile bool            running;
};

/* 消息类型 */

enum mq_msg_type_e
{
  MQ_MSG_ALERT = 1,
  MQ_MSG_TTS,
  MQ_MSG_APP_DATA,
  MQ_MSG_APP_CONFIG,
  MQ_MSG_VOICE_CMD
};

/* 消息结构 */

struct mq_msg_s
{
  enum mq_msg_type_e type;
  char               data[MQ_MSG_SIZE - sizeof(enum mq_msg_type_e)];
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* === algo/ === */

void     filter_init(void);
float    filter_moving_avg(float new_val, float *history, int len);
uint16_t filter_median(uint16_t *buf, int len);

void     fusion_init(void);
void     fusion_update(const struct radar_data_s *radar,
                       const struct distance_data_s *dist,
                       struct posture_result_s *result);

void     posture_init(void);
void     posture_classify(struct desk_health_ctx_s *ctx);
void     posture_update_timers(struct desk_health_ctx_s *ctx);

void     health_stats_init(struct health_stats_s *stats);
void     health_stats_update(struct desk_health_ctx_s *ctx);
void     health_stats_daily_reset(struct health_stats_s *stats);

/* === tasks/ === */

void    *task_radar_main(void *arg);
void    *task_distance_main(void *arg);
void    *task_environment_main(void *arg);
void    *task_esp32_main(void *arg);
void    *task_display_main(void *arg);

/* === protocol/ === */

int      esp32_proto_send(const char *prefix, const char *data);
int      esp32_proto_parse(char *buf, int len,
                           enum mq_msg_type_e *type, char *payload);
int      app_proto_build_json(const struct desk_health_ctx_s *ctx,
                              char *buf, int buflen);
int      app_proto_parse_config(const char *json,
                                struct system_config_s *config);

/* === ui/ === */

void     ui_init(void);
void     ui_draw_main_screen(struct desk_health_ctx_s *ctx);
void     ui_draw_alert_popup(enum alert_level_e level,
                             enum posture_state_e trigger);
void     ui_set_face(enum face_type_e face);
void    *task_led_main(void *arg);
void    *task_button_main(void *arg);
void    *task_storage_main(void *arg);

#endif /* __APPS_EXAMPLES_DESK_HEALTH_INCLUDE_DESK_HEALTH_H */
