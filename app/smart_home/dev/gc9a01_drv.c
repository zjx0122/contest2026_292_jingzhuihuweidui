/****************************************************************************
 * app/smart_home/dev/gc9a01_drv.c
 * GC9A01 圆形SPI屏幕驱动（SPI1: PA5=SCK, PA7=MOSI）
 * CS=PD7, DC=PC4, RST=PC0, BL=PD12
 * 分辨率：240x240
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <nuttx/spi/spi_transfer.h>
#include <sys/ioctl.h>
#include "stm32.h"
#include "stm32f407-custom.h"
#include "smart_home.h"

#define GC9A01_WIDTH   240
#define GC9A01_HEIGHT  240

static int g_spi_fd = -1;

/* 引脚操作宏 */

#define CS_LOW()   stm32_gpiowrite(GPIO_GC9A01_CS, 0)
#define CS_HIGH()  stm32_gpiowrite(GPIO_GC9A01_CS, 1)
#define DC_CMD()   stm32_gpiowrite(GPIO_GC9A01_DC, 0)
#define DC_DATA()  stm32_gpiowrite(GPIO_GC9A01_DC, 1)
#define RST_LOW()  stm32_gpiowrite(GPIO_GC9A01_RST, 0)
#define RST_HIGH() stm32_gpiowrite(GPIO_GC9A01_RST, 1)
#define BL_ON()    stm32_gpiowrite(GPIO_GC9A01_BL, 1)
#define BL_OFF()   stm32_gpiowrite(GPIO_GC9A01_BL, 0)

static void gc9a01_write_cmd(uint8_t cmd)
{
  DC_CMD(); CS_LOW();
  write(g_spi_fd, &cmd, 1);
  CS_HIGH();
}

static void gc9a01_write_data(uint8_t data)
{
  DC_DATA(); CS_LOW();
  write(g_spi_fd, &data, 1);
  CS_HIGH();
}

static void gc9a01_write_data_bulk(const uint8_t *data, int len)
{
  DC_DATA(); CS_LOW();
  write(g_spi_fd, data, len);
  CS_HIGH();
}

static void gc9a01_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  gc9a01_write_cmd(0x2a);
  gc9a01_write_data(x0 >> 8); gc9a01_write_data(x0 & 0xff);
  gc9a01_write_data(x1 >> 8); gc9a01_write_data(x1 & 0xff);
  gc9a01_write_cmd(0x2b);
  gc9a01_write_data(y0 >> 8); gc9a01_write_data(y0 & 0xff);
  gc9a01_write_data(y1 >> 8); gc9a01_write_data(y1 & 0xff);
  gc9a01_write_cmd(0x2c);
}

static void gc9a01_hard_reset(void)
{
  RST_LOW(); usleep(10000);
  RST_HIGH(); usleep(120000);
}

static void gc9a01_init_sequence(void)
{
  gc9a01_hard_reset();
  gc9a01_write_cmd(0xfe); gc9a01_write_cmd(0xef);
  gc9a01_write_cmd(0xb6); gc9a01_write_data(0x00); gc9a01_write_data(0x20);
  gc9a01_write_cmd(0x36); gc9a01_write_data(0x08);
  gc9a01_write_cmd(0x3a); gc9a01_write_data(0x05);
  gc9a01_write_cmd(0xb5); gc9a01_write_data(0x0d); gc9a01_write_data(0x0d);
  gc9a01_write_cmd(0xb4); gc9a01_write_data(0x21);
  gc9a01_write_cmd(0xff); gc9a01_write_data(0x30);
  gc9a01_write_cmd(0xfc); gc9a01_write_data(0x00);
  gc9a01_write_cmd(0x11); usleep(120000);
  gc9a01_write_cmd(0x29); usleep(20000);
}

int gc9a01_init(void)
{
  g_spi_fd = open(DEV_SPI1, O_WRONLY);
  if (g_spi_fd < 0) { syslog(LOG_ERR, "GC9A01: open SPI1 failed\n"); return -errno; }
  gc9a01_init_sequence();
  BL_ON();
  syslog(LOG_INFO, "GC9A01 init OK, 240x240, SPI1(PA5/PA7), CS=PD7 DC=PC4 RST=PC0 BL=PD12\n");
  return OK;
}

int gc9a01_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
  if (g_spi_fd < 0) return -ENODEV;
  gc9a01_set_window(x, y, x + w - 1, y + h - 1);
  uint8_t hi = color >> 8, lo = color & 0xff;
  uint8_t line[480];
  for (int i = 0; i < w && i < 240; i++) { line[i*2] = hi; line[i*2+1] = lo; }
  for (int j = 0; j < h; j++) gc9a01_write_data_bulk(line, w * 2);
  return OK;
}

int gc9a01_clear(uint16_t color)
{
  return gc9a01_fill_rect(0, 0, GC9A01_WIDTH, GC9A01_HEIGHT, color);
}

int gc9a01_show_string(uint16_t x, uint16_t y, const char *str, uint16_t color)
{
  /* 简易字符显示，实际项目建议用字库 */
  if (!str) return -EINVAL;
  syslog(LOG_INFO, "GC9A01 show at(%d,%d): %s\n", x, y, str);
  return OK;
}
