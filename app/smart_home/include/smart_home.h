/****************************************************************************
 * app/smart_home/include/smart_home.h
 * 智能家居系统公共头文件
 * 硬件：GC9A01屏/SHT30+VL53L0X/LD2410B/SU-03T/BLE
 ****************************************************************************/

#ifndef __SMART_HOME_H
#define __SMART_HOME_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * 设备路径定义
 ****************************************************************************/

#define DEV_USART1      "/dev/ttyS0"   /* 调试串口 */
#define DEV_USART2      "/dev/ttyS1"   /* LD2410B雷达 */
#define DEV_USART3      "/dev/ttyS2"   /* SU-03T语音 */
#define DEV_UART4       "/dev/ttyS3"   /* BLE蓝牙 */
#define DEV_I2C1        "/dev/i2c1"    /* SHT30+VL53L0X */
#define DEV_SPI1        "/dev/spi1"    /* GC9A01+W25Q */

/****************************************************************************
 * 传感器数据结构
 ****************************************************************************/

/* SHT30温湿度数据 */

struct sht30_data_s
{
  float temperature;    /* 温度，单位：摄氏度 */
  float humidity;       /* 湿度，单位：%RH */
  uint64_t timestamp;   /* 时间戳 */
};

/* VL53L0X测距数据 */

struct vl53l0x_data_s
{
  uint16_t distance_mm; /* 距离，单位：毫米 */
  uint8_t status;       /* 测量状态 */
  uint64_t timestamp;
};

/* LD2410B雷达数据 */

struct ld2410b_data_s
{
  bool target_detected;  /* 是否检测到目标 */
  uint16_t distance_cm;  /* 目标距离，单位：厘米 */
  uint8_t energy;        /* 能量值 */
  uint64_t timestamp;
};

/* 综合传感器数据 */

struct sensor_data_s
{
  struct sht30_data_s   sht30;
  struct vl53l0x_data_s vl53l0x;
  struct ld2410b_data_s  ld2410b;
};

/****************************************************************************
 * SU-03T语音指令定义
 ****************************************************************************/

enum su03t_cmd_e
{
  SU03T_CMD_NONE = 0,
  SU03T_CMD_LIGHT_ON,       /* 打开灯 */
  SU03T_CMD_LIGHT_OFF,      /* 关闭灯 */
  SU03T_CMD_FAN_ON,         /* 打开风扇 */
  SU03T_CMD_FAN_OFF,        /* 关闭风扇 */
  SU03T_CMD_READ_TEMP,      /* 读取温度 */
  SU03T_CMD_READ_HUMIDITY,  /* 读取湿度 */
  SU03T_CMD_READ_DISTANCE,  /* 读取距离 */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* dev层 - 设备驱动 */

int gc9a01_init(void);
int gc9a01_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     uint16_t color);
int gc9a01_clear(uint16_t color);
int gc9a01_show_string(uint16_t x, uint16_t y, const char *str,
                       uint16_t color);

int sht30_init(void);
int sht30_read(struct sht30_data_s *data);

int vl53l0x_init(void);
int vl53l0x_read(struct vl53l0x_data_s *data);

int ld2410b_init(void);
int ld2410b_read(struct ld2410b_data_s *data);

int su03t_init(void);
enum su03t_cmd_e su03t_read_cmd(void);
int su03t_play_text(const char *text);

/* service层 - 业务服务 */

int sensor_service_init(void);
int sensor_service_read(struct sensor_data_s *data);

int display_service_init(void);
int display_service_update(const struct sensor_data_s *data);

int comm_service_init(void);
int comm_service_send(const char *data, int len);

/* app层 - 应用入口 */

int smart_home_main(int argc, char *argv[]);

#endif /* __SMART_HOME_H */