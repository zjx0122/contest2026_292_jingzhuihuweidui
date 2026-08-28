/****************************************************************************
 * boards/arm/stm32/stm32f407_health/src/stm32_sht30.c
 *
 * SHT30 温湿度传感器驱动
 * I2C1: PB6=SCL, PB9=SDA, 地址 0x44
 *
 ****************************************************************************/

#include <nuttx/config.h>
#include <errno.h>
#include <debug.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <nuttx/i2c/i2c_master.h>
#include "stm32_i2c.h"
#include "stm32.h"

#ifdef CONFIG_SENSORS_SHT30

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SHT30_I2C_ADDR      0x44
#define SHT30_I2C_FREQ      100000

/* SHT30 命令定义 */

#define SHT30_CMD_RESET_H   0x30
#define SHT30_CMD_RESET_L   0xa2
#define SHT30_CMD_PERIODIC_H  0x21
#define SHT30_CMD_PERIODIC_L  0x30
#define SHT30_CMD_READ_H    0xe0
#define SHT30_CMD_READ_L    0x00

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct i2c_master_s *g_i2c1 = NULL;

static const struct i2c_config_s g_sht30_config =
{
  .frequency = SHT30_I2C_FREQ,
  .address   = SHT30_I2C_ADDR,
  .addrlen   = 7
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint8_t sht30_crc8(const uint8_t *data, int len)
{
  uint8_t crc = 0xff;
  int i;
  int bit;

  for (i = 0; i < len; i++)
    {
      crc ^= data[i];

      for (bit = 0; bit < 8; bit++)
        {
          if (crc & 0x80)
            {
              crc = (uint8_t)((crc << 1) ^ 0x31);
            }
          else
            {
              crc <<= 1;
            }
        }
    }

  return crc;
}

static int sht30_reset(void)
{
  uint8_t command[2] = {SHT30_CMD_RESET_H, SHT30_CMD_RESET_L};
  int ret;

  ret = i2c_write(g_i2c1, &g_sht30_config, command, sizeof(command));
  if (ret < 0)
    {
      serr("ERROR: SHT30 复位失败: %d\n", ret);
      return ret;
    }

  usleep(5000);
  return OK;
}

static int sht30_start_periodic(void)
{
  uint8_t command[2] = {SHT30_CMD_PERIODIC_H, SHT30_CMD_PERIODIC_L};

  return i2c_write(g_i2c1, &g_sht30_config, command, sizeof(command));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_sht30_setup(void)
{
  int ret;

  /* 初始化 I2C1 */

  g_i2c1 = stm32_i2cbus_initialize(1);
  if (g_i2c1 == NULL)
    {
      serr("ERROR: 初始化 I2C1 失败\n");
      return -ENODEV;
    }

  /* 复位 SHT30 */

  ret = sht30_reset();
  if (ret < 0)
    {
      return ret;
    }

  /* 启动周期性测量 */

  ret = sht30_start_periodic();
  if (ret < 0)
    {
      serr("ERROR: SHT30 启动周期性测量失败: %d\n", ret);
      return ret;
    }

  sinfo("SHT30 温湿度传感器初始化成功 (I2C1, 地址 0x%02X)\n", SHT30_I2C_ADDR);
  return OK;
}

int stm32_sht30_read(int32_t *temperature, uint32_t *humidity)
{
  uint8_t command[2] = {SHT30_CMD_READ_H, SHT30_CMD_READ_L};
  uint8_t data[6];
  uint8_t temp_crc;
  uint8_t humi_crc;
  uint16_t temp_raw;
  uint16_t humi_raw;
  int ret;

  if (temperature == NULL || humidity == NULL)
    {
      return -EINVAL;
    }

  if (g_i2c1 == NULL)
    {
      return -ENODEV;
    }

  /* 发送读取命令 */

  ret = i2c_write(g_i2c1, &g_sht30_config, command, sizeof(command));
  if (ret < 0)
    {
      return ret;
    }

  usleep(50000); /* 等待测量完成 */

  /* 读取 6 字节数据: [temp_H, temp_L, temp_CRC, humi_H, humi_L, humi_CRC] */

  ret = i2c_read(g_i2c1, &g_sht30_config, data, sizeof(data));
  if (ret < 0)
    {
      return ret;
    }

  /* CRC 校验 */

  temp_crc = sht30_crc8(data, 2);
  humi_crc = sht30_crc8(data + 3, 2);

  if (temp_crc != data[2] || humi_crc != data[5])
    {
      serr("ERROR: SHT30 CRC 校验失败\n");
      return -EIO;
    }

  /* 计算温度: T = -45 + 175 * raw / 65535 */

  temp_raw = ((uint16_t)data[0] << 8) | data[1];
  *temperature = (int32_t)(-4500 + (int32_t)(17500 * (int32_t)temp_raw / 65535));

  /* 计算湿度: RH = 100 * raw / 65535 */

  humi_raw = ((uint16_t)data[3] << 8) | data[4];
  *humidity = (uint32_t)(10000 * (uint32_t)humi_raw / 65535);

  return OK;
}

int stm32_sht30_get_temperature(int32_t *temperature)
{
  uint32_t humidity;
  return stm32_sht30_read(temperature, &humidity);
}

int stm32_sht30_get_humidity(uint32_t *humidity)
{
  int32_t temperature;
  return stm32_sht30_read(&temperature, humidity);
}

int stm32_sht30_read_periodic(int32_t *temperature, uint32_t *humidity)
{
  uint8_t command[2] = {SHT30_CMD_READ_H, SHT30_CMD_READ_L};
  uint8_t data[6];
  uint16_t temp_raw;
  uint16_t humi_raw;
  int ret;

  if (temperature == NULL || humidity == NULL)
    {
      return -EINVAL;
    }

  if (g_i2c1 == NULL)
    {
      return -ENODEV;
    }

  /* 发送读取周期性测量结果命令 (0xe000) */

  ret = i2c_write(g_i2c1, &g_sht30_config, command, sizeof(command));
  if (ret < 0)
    {
      return ret;
    }

  /* 读取 6 字节数据 */

  ret = i2c_read(g_i2c1, &g_sht30_config, data, sizeof(data));
  if (ret < 0)
    {
      return ret;
    }

  /* CRC 校验 */

  if (sht30_crc8(data, 2) != data[2] ||
      sht30_crc8(data + 3, 2) != data[5])
    {
      serr("ERROR: SHT30 CRC 校验失败\n");
      return -EIO;
    }

  /* 计算温湿度 */

  temp_raw = ((uint16_t)data[0] << 8) | data[1];
  humi_raw = ((uint16_t)data[3] << 8) | data[4];

  *temperature = -4500 + (int32_t)(((uint32_t)17500 * temp_raw) / 65535);
  *humidity = ((uint32_t)10000 * humi_raw) / 65535;

  return OK;
}

#endif /* CONFIG_SENSORS_SHT30 */
