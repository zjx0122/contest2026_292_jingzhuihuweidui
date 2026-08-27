/****************************************************************************
 * boards/arm/stm32/stm32f407_health/src/stm32_gc9a01.c
 *
 * GC9A01 圆形 TFT LCD 板级驱动
 * SPI1: PB3=SCK, PB5=MOSI, CS=未接, DC=PD7, RST=PA4
 *
 * 基于 nuttx/boards/arm/rp2040/common/src/rp2040_gc9a01.c 移植
 ****************************************************************************/

#include <nuttx/config.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/spi/spi.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/gc9a01.h>

#include "stm32_gpio.h"
#include "stm32.h"
#include <arch/board/board.h>

#ifdef CONFIG_LCD_GC9A01

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SPI1 端口号 */

#define GC9A01_SPI_PORTNO 1

#ifndef CONFIG_SPI_CMDDATA
#  error "GC9A01 驱动需要 CONFIG_SPI_CMDDATA"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct spi_dev_s *g_gc9a01_spidev;
static struct lcd_dev_s *g_gc9a01_lcd = NULL;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_lcd_initialize
 *
 * Description:
 *   初始化 GC9A01 LCD 硬件。
 *   初始化 SPI 总线，配置 DC/RST 引脚，执行硬件复位序列。
 *   由 fb_register() 内部调用，不要在其他地方重复调用。
 *
 ****************************************************************************/

int board_lcd_initialize(void)
{
  /* 获取 SPI1 总线 */

  g_gc9a01_spidev = stm32_spibus_initialize(GC9A01_SPI_PORTNO);
  if (!g_gc9a01_spidev)
    {
      lcderr("ERROR: 初始化 SPI%d 失败\n", GC9A01_SPI_PORTNO);
      return -ENODEV;
    }

  /* 配置 DC 引脚（数据/命令） */

  stm32_configgpio(GPIO_LCD_DC);
  stm32_gpiowrite(GPIO_LCD_DC, false);

  /* 配置 RST 引脚并执行复位序列 */

  stm32_configgpio(GPIO_LCD_RST);
  stm32_gpiowrite(GPIO_LCD_RST, false);
  up_mdelay(50);
  stm32_gpiowrite(GPIO_LCD_RST, true);
  up_mdelay(50);

  lcdinfo("GC9A01 LCD 硬件初始化完成 (SPI1: PB3=SCK, PB5=MOSI)\n");
  return OK;
}

/****************************************************************************
 * Name: board_lcd_getdev
 *
 * Description:
 *   获取 LCD 设备实例。
 *   由 fb_register() 内部调用。
 *
 ****************************************************************************/

struct lcd_dev_s *board_lcd_getdev(int devno)
{
  g_gc9a01_lcd = gc9a01_lcdinitialize(g_gc9a01_spidev);
  if (!g_gc9a01_lcd)
    {
      lcderr("ERROR: GC9A01 LCD 绑定 SPI%d 失败\n", GC9A01_SPI_PORTNO);
    }
  else
    {
      lcdinfo("GC9A01 LCD 绑定到设备 %d 成功\n", devno);
      return g_gc9a01_lcd;
    }

  return NULL;
}

/****************************************************************************
 * Name: board_lcd_uninitialize
 *
 * Description:
 *   关闭 LCD 显示。
 *
 ****************************************************************************/

void board_lcd_uninitialize(void)
{
  if (g_gc9a01_lcd)
    {
      g_gc9a01_lcd->setpower(g_gc9a01_lcd, 0);
    }
}

/****************************************************************************
 * Name: stm32_gc9a01_setup
 *
 * Description:
 *   板级初始化入口，供 stm32_bringup.c 调用。
 *   注册 framebuffer 设备 /dev/fb0。
 *   fb_register() 内部会调用 board_lcd_initialize() 和 board_lcd_getdev()。
 *
 ****************************************************************************/

int stm32_gc9a01_setup(void)
{
  int ret;

  /* 注册 framebuffer 设备 /dev/fb0
   * fb_register() 内部会自动调用 board_lcd_initialize() 初始化硬件，
   * 再调用 board_lcd_getdev() 获取 LCD 设备实例。
   */

  ret = fb_register(0, 0);
  if (ret < 0)
    {
      lcderr("ERROR: fb_register(0, 0) 失败: %d\n", ret);
      return ret;
    }

  lcdinfo("GC9A01 LCD 初始化完成，已注册 /dev/fb0\n");
  return OK;
}

#endif /* CONFIG_LCD_GC9A01 */
