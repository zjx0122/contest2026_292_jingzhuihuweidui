/****************************************************************************
 * app/smart_home/service/sensor_service.c
 * 传感器融合服务：统一管理SHT30/VL53L0X/LD2410B数据采集
 ****************************************************************************/

#include <nuttx/config.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include "smart_home.h"

static struct sensor_data_s g_sensor_data;

int sensor_service_init(void)
{
  int ret = OK;
  memset(&g_sensor_data, 0, sizeof(g_sensor_data));
  ret = sht30_init();
  if (ret < 0) syslog(LOG_WARNING, "SHT30 init failed: %d\n", ret);
  ret = vl53l0x_init();
  if (ret < 0) syslog(LOG_WARNING, "VL53L0X init failed: %d\n", ret);
  ret = ld2410b_init();
  if (ret < 0) syslog(LOG_WARNING, "LD2410B init failed: %d\n", ret);
  syslog(LOG_INFO, "Sensor service initialized\n");
  return OK;
}

int sensor_service_read(struct sensor_data_s *data)
{
  if (!data) return -EINVAL;
  sht30_read(&g_sensor_data.sht30);
  vl53l0x_read(&g_sensor_data.vl53l0x);
  ld2410b_read(&g_sensor_data.ld2410b);
  memcpy(data, &g_sensor_data, sizeof(struct sensor_data_s));
  return OK;
}
