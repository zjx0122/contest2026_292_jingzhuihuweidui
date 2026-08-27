/****************************************************************************
 * app/smart_home/dev/sht30_drv.c
 * SHT30 温湿度传感器驱动（I2C1: PB6=SCL, PB7=SDA, AF4）
 * I2C地址：0x44（ADDR引脚接地）或 0x45（ADDR引脚接VCC）
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/i2c/i2c_master.h>
#include <sys/ioctl.h>

#include "smart_home.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SHT30 I2C地址（7-bit），ADDR引脚接地时为0x44 */

#define SHT30_I2C_ADDR          0x44
#define SHT30_I2C_FREQ          100000  /* 100kHz */

/* SHT30 命令定义 */

#define SHT30_CMD_MEASURE_HIGH  0x2400  /* 高重复性单次测量 */
#define SHT30_CMD_MEASURE_MED   0x240B  /* 中重复性单次测量 */
#define SHT30_CMD_MEASURE_LOW   0x2416  /* 低重复性单次测量 */
#define SHT30_CMD_SOFT_RESET    0x30A2  /* 软复位 */

/* CRC多项式 */

#define SHT30_CRC_POLYNOMIAL    0x31

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_sht30_fd = -1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sht30_crc8
 *
 * Description:
 *   计算SHT30 CRC-8校验值
 *
 ****************************************************************************/

static uint8_t sht30_crc8(const uint8_t *data, int len)
{
  uint8_t crc = 0xff;
  int i;
  int j;

  for (i = 0; i < len; i++)
    {
      crc ^= data[i];
      for (j = 0; j < 8; j++)
        {
          if (crc & 0x80)
            {
              crc = (crc << 1) ^ SHT30_CRC_POLYNOMIAL;
            }
          else
            {
              crc = crc << 1;
            }
        }
    }

  return crc;
}

/****************************************************************************
 * Name: sht30_write_cmd
 *
 * Description:
 *   向SHT30发送命令
 *
 ****************************************************************************/

static int sht30_write_cmd(uint16_t cmd)
{
  uint8_t buf[2];
  struct i2c_msg_s msg;
  struct i2c_transfer_s xfer;

  buf[0] = (cmd >> 8) & 0xff;
  buf[1] = cmd & 0xff;

  msg.addr   = SHT30_I2C_ADDR;
  msg.flags  = 0;  /* 写操作 */
  msg.buffer = buf;
  msg.length = 2;

  xfer.msgv  = &msg;
  xfer.msgc  = 1;

  if (g_sht30_fd < 0)
    {
      return -ENODEV;
    }

  return ioctl(g_sht30_fd, I2C_TRANSFER, (unsigned long)&xfer);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sht30_init
 *
 * Description:
 *   初始化SHT30温湿度传感器
 *   打开I2C1设备，发送软复位命令
 *
 ****************************************************************************/

int sht30_init(void)
{
  int ret;

  /* 打开I2C1设备 */

  g_sht30_fd = open(DEV_I2C1, O_RDWR);
  if (g_sht30_fd < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to open %s: %d\n",
             DEV_I2C1, errno);
      return -errno;
    }

  /* 发送软复位命令 */

  ret = sht30_write_cmd(SHT30_CMD_SOFT_RESET);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: SHT30 soft reset failed: %d\n", ret);
      close(g_sht30_fd);
      g_sht30_fd = -1;
      return ret;
    }

  /* 等待复位完成 */

  usleep(10000);  /* 10ms */

  syslog(LOG_INFO, "SHT30 initialized on I2C1 (PB6=SCL, PB7=SDA)\n");
  return OK;
}

/****************************************************************************
 * Name: sht30_read
 *
 * Description:
 *   读取SHT30温湿度数据
 *
 * Input Parameters:
 *   data - 指向数据结构的指针
 *
 * Returned Value:
 *   0: 成功
 *   负数: 失败
 *
 ****************************************************************************/

int sht30_read(struct sht30_data_s *data)
{
  int ret;
  uint8_t raw[6];
  uint8_t cmd[2];
  struct i2c_msg_s msgs[2];
  struct i2c_transfer_s xfer;

  if (data == NULL || g_sht30_fd < 0)
    {
      return -EINVAL;
    }

  /* 发送测量命令 */

  cmd[0] = (SHT30_CMD_MEASURE_HIGH >> 8) & 0xff;
  cmd[1] = SHT30_CMD_MEASURE_HIGH & 0xff;

  msgs[0].addr   = SHT30_I2C_ADDR;
  msgs[0].flags  = 0;  /* 写 */
  msgs[0].buffer = cmd;
  msgs[0].length = 2;

  xfer.msgv = &msgs[0];
  xfer.msgc = 1;

  ret = ioctl(g_sht30_fd, I2C_TRANSFER, (unsigned long)&xfer);
  if (ret < 0)
    {
      return ret;
    }

  /* 等待测量完成（高重复性约15ms） */

  usleep(20000);

  /* 读取6字节数据：Temp[2] + CRC + Hum[2] + CRC */

  msgs[0].addr   = SHT30_I2C_ADDR;
  msgs[0].flags  = I2C_M_READ;
  msgs[0].buffer = raw;
  msgs[0].length = 6;

  xfer.msgv = &msgs[0];
  xfer.msgc = 1;

  ret = ioctl(g_sht30_fd, I2C_TRANSFER, (unsigned long)&xfer);
  if (ret < 0)
    {
      return ret;
    }

  /* 校验CRC */

  if (sht30_crc8(raw, 2) != raw[2] ||
      sht30_crc8(raw + 3, 2) != raw[5])
    {
      syslog(LOG_WARNING, "SHT30 CRC check failed\n");
      return -EIO;
    }

  /* 计算温度和湿度 */

  uint16_t raw_temp = (raw[0] << 8) | raw[1];
  uint16_t raw_hum  = (raw[3] << 8) | raw[4];

  data->temperature = -45.0f + 175.0f * (float)raw_temp / 65535.0f;
  data->humidity    = 100.0f * (float)raw_hum / 65535.0f;

  /* 获取时间戳 */

  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  data->timestamp = (uint64_t)ts.tv_sec * 1000000ULL +
                    (uint64_t)ts.tv_nsec / 1000ULL;

  return OK;
}