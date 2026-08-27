/****************************************************************************
 * app/smart_home/dev/vl53l0x_drv.c
 * VL53L0X 激光测距传感器驱动（I2C1: PB6=SCL, PB7=SDA, AF4）
 * XSHUT=PC5（低电平休眠，高电平工作）
 * I2C地址：0x29（默认）
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

#define VL53L0X_I2C_ADDR        0x29
#define VL53L0X_REG_MODEL_ID    0xc0
#define VL53L0X_REG_SYSRANGE    0x00
#define VL53L0X_REG_RESULT      0x14

static int g_fd = -1;

static int vl_read_reg(uint8_t reg, uint8_t *val)
{
  struct i2c_msg_s msgs[2];
  struct i2c_transfer_s xfer;
  msgs[0].addr = VL53L0X_I2C_ADDR; msgs[0].flags = 0;
  msgs[0].buffer = &reg; msgs[0].length = 1;
  msgs[1].addr = VL53L0X_I2C_ADDR; msgs[1].flags = I2C_M_READ;
  msgs[1].buffer = val; msgs[1].length = 1;
  xfer.msgv = msgs; xfer.msgc = 2;
  return ioctl(g_fd, I2C_TRANSFER, (unsigned long)&xfer);
}

static int vl_write_reg(uint8_t reg, uint8_t val)
{
  uint8_t buf[2] = {reg, val};
  struct i2c_msg_s msg;
  struct i2c_transfer_s xfer;
  msg.addr = VL53L0X_I2C_ADDR; msg.flags = 0;
  msg.buffer = buf; msg.length = 2;
  xfer.msgv = &msg; xfer.msgc = 1;
  return ioctl(g_fd, I2C_TRANSFER, (unsigned long)&xfer);
}

static int vl_read_reg16(uint8_t reg, uint16_t *val)
{
  uint8_t buf[2];
  struct i2c_msg_s msgs[2];
  struct i2c_transfer_s xfer;
  msgs[0].addr = VL53L0X_I2C_ADDR; msgs[0].flags = 0;
  msgs[0].buffer = &reg; msgs[0].length = 1;
  msgs[1].addr = VL53L0X_I2C_ADDR; msgs[1].flags = I2C_M_READ;
  msgs[1].buffer = buf; msgs[1].length = 2;
  xfer.msgv = msgs; xfer.msgc = 2;
  int ret = ioctl(g_fd, I2C_TRANSFER, (unsigned long)&xfer);
  if (ret < 0) return ret;
  *val = (buf[0] << 8) | buf[1];
  return OK;
}

int vl53l0x_init(void)
{
  uint8_t id;
  g_fd = open(DEV_I2C1, O_RDWR);
  if (g_fd < 0) { syslog(LOG_ERR, "VL53L0X: open I2C1 failed\n"); return -errno; }
  if (vl_read_reg(VL53L0X_REG_MODEL_ID, &id) < 0)
    {
      syslog(LOG_ERR, "VL53L0X: not responding, check XSHUT(PC5)=HIGH\n");
      close(g_fd); g_fd = -1; return -ENODEV;
    }
  syslog(LOG_INFO, "VL53L0X init OK, ID=0x%02x, XSHUT=PC5\n", id);
  return OK;
}

int vl53l0x_read(struct vl53l0x_data_s *data)
{
  uint8_t st; uint16_t range; int retry = 50;
  if (!data || g_fd < 0) return -EINVAL;
  vl_write_reg(VL53L0X_REG_SYSRANGE, 0x01);
  do { usleep(5000); vl_read_reg(VL53L0X_REG_SYSRANGE, &st); retry--; }
  while ((st & 0x01) && retry > 0);
  if (!retry) return -ETIMEDOUT;
  vl_read_reg16(VL53L0X_REG_RESULT + 10, &range);
  data->distance_mm = range;
  data->status = (range > 0 && range < 2000) ? 0 : 1;
  struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
  data->timestamp = (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL;
  return OK;
}
