/****************************************************************************
 * app/smart_home/app/smart_home_main.c
 * 智能家居主应用
 * 功能：采集传感器数据、更新显示屏、处理语音指令、BLE通信
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include "smart_home.h"

static bool g_running = true;

static void handle_voice_cmd(enum su03t_cmd_e cmd, const struct sensor_data_s *data)
{
  char buf[64];
  switch (cmd)
    {
      case SU03T_CMD_LIGHT_ON:
        stm32_gpiowrite(GPIO_LED1, true);
        su03t_play_text("灯已打开");
        break;
      case SU03T_CMD_LIGHT_OFF:
        stm32_gpiowrite(GPIO_LED1, false);
        su03t_play_text("灯已关闭");
        break;
      case SU03T_CMD_READ_TEMP:
        snprintf(buf, sizeof(buf), "当前温度%.1f度", data->sht30.temperature);
        su03t_play_text(buf);
        break;
      case SU03T_CMD_READ_HUMIDITY:
        snprintf(buf, sizeof(buf), "当前湿度%.1f%%", data->sht30.humidity);
        su03t_play_text(buf);
        break;
      case SU03T_CMD_READ_DISTANCE:
        snprintf(buf, sizeof(buf), "距离%u毫米", data->vl53l0x.distance_mm);
        su03t_play_text(buf);
        break;
      default:
        break;
    }
}

int smart_home_main(int argc, char *argv[])
{
  int ret;
  struct sensor_data_s data;
  enum su03t_cmd_e cmd;

  syslog(LOG_INFO, "=== Smart Home System Starting ===\n");
  syslog(LOG_INFO, "Hardware: STM32F407VGT6 + Expansion Board\n");

  /* 初始化各层 */

  ret = display_service_init();
  if (ret < 0) syslog(LOG_WARNING, "Display service init failed\n");

  ret = sensor_service_init();
  if (ret < 0) syslog(LOG_WARNING, "Sensor service init failed\n");

  ret = su03t_init();
  if (ret < 0) syslog(LOG_WARNING, "SU-03T init failed\n");

  ret = comm_service_init();
  if (ret < 0) syslog(LOG_WARNING, "BLE comm init failed\n");

  syslog(LOG_INFO, "=== All services started, entering main loop ===\n");

  /* 主循环 */

  while (g_running)
    {
      /* 1. 读取所有传感器 */
      sensor_service_read(&data);

      /* 2. 更新显示 */
      display_service_update(&data);

      /* 3. 处理语音指令 */
      cmd = su03t_read_cmd();
      if (cmd != SU03T_CMD_NONE)
        handle_voice_cmd(cmd, &data);

      /* 4. BLE上报数据（JSON格式）*/
      {
        char json[128];
        snprintf(json, sizeof(json),
                 "{\"t\":%.1f,\"h\":%.1f,\"d\":%u,\"r\":%d}",
                 data.sht30.temperature, data.sht30.humidity,
                 data.vl53l0x.distance_mm,
                 data.ld2410b.target_detected ? 1 : 0);
        comm_service_send(json, strlen(json));
      }

      usleep(500000);  /* 500ms采样周期 */
    }

  syslog(LOG_INFO, "=== Smart Home System Stopped ===\n");
  return OK;
}
