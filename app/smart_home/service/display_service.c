/****************************************************************************
 * app/smart_home/service/display_service.c
 * 显示服务：在GC9A01圆形屏上显示传感器数据
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include "smart_home.h"

#define COLOR_BG     0x0000  /* 黑色背景 */
#define COLOR_TEXT   0xFFFF  /* 白色文字 */
#define COLOR_GREEN  0x07E0  /* 绿色 */
#define COLOR_YELLOW 0xFFE0  /* 黄色 */
#define COLOR_RED    0xF800  /* 红色 */

int display_service_init(void)
{
  int ret = gc9a01_init();
  if (ret < 0) { syslog(LOG_ERR, "Display init failed: %d\n", ret); return ret; }
  gc9a01_clear(COLOR_BG);
  gc9a01_show_string(20, 20, "Smart Home System", COLOR_TEXT);
  syslog(LOG_INFO, "Display service initialized\n");
  return OK;
}

int display_service_update(const struct sensor_data_s *data)
{
  char buf[64];
  if (!data) return -EINVAL;
  /* 温湿度区域 */
  gc9a01_fill_rect(10, 60, 220, 30, COLOR_BG);
  snprintf(buf, sizeof(buf), "T:%.1fC H:%.1f%%", data->sht30.temperature, data->sht30.humidity);
  gc9a01_show_string(10, 60, buf, COLOR_GREEN);
  /* 距离区域 */
  gc9a01_fill_rect(10, 100, 220, 30, COLOR_BG);
  snprintf(buf, sizeof(buf), "Dist:%umm", data->vl53l0x.distance_mm);
  gc9a01_show_string(10, 100, buf, COLOR_YELLOW);
  /* 雷达区域 */
  gc9a01_fill_rect(10, 140, 220, 30, COLOR_BG);
  snprintf(buf, sizeof(buf), "Radar:%s %ucm",
           data->ld2410b.target_detected ? "YES" : "NO",
           data->ld2410b.distance_cm);
  gc9a01_show_string(10, 140, buf, data->ld2410b.target_detected ? COLOR_RED : COLOR_TEXT);
  return OK;
}
