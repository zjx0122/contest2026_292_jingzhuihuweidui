/****************************************************************************
 * apps/examples/desk_health/tasks/task_esp32_comm.c
 *
 * ESP32-S3 通信任务
 * 功能：发送传感器数据到 ESP32-S3，接收控制命令
 *
 * 硬件连接：
 *   STM32 USART3 (PB10=TX, PB11=RX) ↔ ESP32-S3 (GPIO18=RX, GPIO17=TX)
 *   波特率：115200
 *
 * 数据格式（JSON）：
 *   STM32 → ESP32-S3:
 *   {
 *     "temp": 25.3,     // 温度 (°C)
 *     "humi": 60.2,     // 湿度 (%)
 *     "dist": 150,      // 距离 (mm)
 *     "human": 1        // 是否有人 (0/1)
 *   }
 *
 *   ESP32-S3 → STM32:
 *   {"cmd":"led","val":1}      // LED控制
 *   {"cmd":"query"}            // 查询数据
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <nuttx/arch.h>
#include <arch/board/board.h>
#include "desk_health.h"

#ifdef CONFIG_DESK_HEALTH_ESP32

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ESP32_COMM_INTERVAL_MS  1000  /* 1秒发送一次 */
#define ESP32_RECV_TIMEOUT_MS   100   /* 接收超时 100ms */
#define ESP32_JSON_BUF_SIZE     128
#define ESP32_CMD_BUF_SIZE      64

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int esp32_send_sensor_data(const struct desk_health_data_s *data)
{
  char json[ESP32_JSON_BUF_SIZE];
  int len;
  int ret;

  if (data == NULL)
    {
      return -EINVAL;
    }

  len = snprintf(json, sizeof(json),
                 "{\"temp\":%.1f,\"humi\":%.1f,\"dist\":%u,\"human\":%d}",
                 data->temperature,
                 data->humidity,
                 data->distance_mm,
                 data->human_detected ? 1 : 0);

  if (len <= 0 || len >= sizeof(json))
    {
      syslog(LOG_ERR, "ESP32: JSON 打包失败\n");
      return -ENOMEM;
    }

  ret = stm32_esp32s3_send((uint8_t *)json, len);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ESP32: 发送数据失败: %d\n", ret);
      return ret;
    }

  syslog(LOG_DEBUG, "ESP32 TX: %s\n", json);
  return OK;
}

static int esp32_process_command(const char *cmd,
                                 struct desk_health_data_s *data)
{
  if (cmd == NULL)
    {
      return -EINVAL;
    }

  syslog(LOG_INFO, "ESP32 RX: %s\n", cmd);

  if (strstr(cmd, "led_on") != NULL)
    {
      stm32_gpiowrite(GPIO_LED1, true);
      syslog(LOG_INFO, "ESP32 CMD: LED ON\n");
    }
  else if (strstr(cmd, "led_off") != NULL)
    {
      stm32_gpiowrite(GPIO_LED1, false);
      syslog(LOG_INFO, "ESP32 CMD: LED OFF\n");
    }
  else if (strstr(cmd, "query") != NULL)
    {
      syslog(LOG_INFO, "ESP32 CMD: QUERY\n");
      esp32_send_sensor_data(data);
    }
  else
    {
      syslog(LOG_WARNING, "ESP32: 未知命令: %s\n", cmd);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int task_esp32_comm(int argc, char *argv[])
{
  struct desk_health_data_s *data;
  uint8_t rx_buf[ESP32_CMD_BUF_SIZE];
  int ret;

  syslog(LOG_INFO, "=== ESP32-S3 通信任务启动 ===\n");

  ret = stm32_esp32s3_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: ESP32-S3 串口初始化失败: %d\n", ret);
      return ret;
    }

  syslog(LOG_INFO, "ESP32-S3 串口初始化成功\n");

  while (1)
    {
      data = desk_health_get_data();
      if (data == NULL)
        {
          syslog(LOG_WARNING, "WARNING: 获取传感器数据失败\n");
          usleep(ESP32_COMM_INTERVAL_MS * 1000);
          continue;
        }

      esp32_send_sensor_data(data);

      memset(rx_buf, 0, sizeof(rx_buf));
      ret = stm32_esp32s3_recv_timeout(rx_buf,
                                        sizeof(rx_buf) - 1,
                                        ESP32_RECV_TIMEOUT_MS);
      if (ret > 0)
        {
          rx_buf[ret] = '\0';
          esp32_process_command((char *)rx_buf, data);
        }
      else if (ret == -ETIMEDOUT)
        {
          syslog(LOG_DEBUG, "ESP32: 接收超时\n");
        }
      else if (ret < 0)
        {
          syslog(LOG_WARNING, "ESP32: 接收数据错误: %d\n", ret);
        }

      usleep(ESP32_COMM_INTERVAL_MS * 1000);
    }

  return OK;
}

#endif /* CONFIG_DESK_HEALTH_ESP32 */

