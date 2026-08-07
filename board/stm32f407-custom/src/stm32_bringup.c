/****************************************************************************
 * boards/arm/stm32/stm32f407-custom/src/stm32_bringup.c
 * STM32F407VGT6 板级初始化
 * 硬件：GC9A01屏/SHT30+VL53L0X/LD2410B/SU-03T/BLE/W25Q/SD卡
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>
#include <stdio.h>
#include <debug.h>
#include <errno.h>
#include <nuttx/fs/fs.h>

#include "stm32.h"

#ifdef CONFIG_STM32_ROMFS
#  include "stm32_romfs.h"
#endif

#ifdef CONFIG_STM32_OTGFS
#  include "stm32_usbhost.h"
#endif

#ifdef CONFIG_INPUT_BUTTONS
#  include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_USERLED
#  include <nuttx/leds/userled.h>
#endif

#include "stm32f407-custom.h"

#ifdef HAVE_RTC_DRIVER
#  include <nuttx/timers/rtc.h>
#  include "stm32_rtc.h"
#endif

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 *   Bring up board features for STM32F407VGT6.
 *   初始化顺序很重要：
 *   1. 先拉高VL53L0X XSHUT（PC5），确保传感器退出休眠
 *   2. 再初始化I2C/SPI/UART等外设
 ****************************************************************************/

int stm32_bringup(void)
{
  int ret = OK;

  /* ============================================================
   * 第0步：直接点亮 PA1 LED（最优先，确认板子活着）
   * ============================================================ */

  stm32_configgpio(GPIO_LED1);
  stm32_gpiowrite(GPIO_LED1, true);

  /* ============================================================
   * 第1步：VL53L0X XSHUT引脚初始化（必须在I2C之前！）
   * XSHUT=PC5，低电平休眠，高电平工作
   * 上电后必须先拉高XSHUT，再去初始化I2C、读取传感器ID
   * 否则会出现I2C无应答、传感器不工作
   * GPIO1=PC4(输入，测距完成中断，轮询模式可不用)
   * ============================================================ */

  stm32_configgpio(GPIO_VL53L0X_XSHUT);
  stm32_gpiowrite(GPIO_VL53L0X_XSHUT, true);  /* 拉高使能传感器 */
  stm32_configgpio(GPIO_VL53L0X_GPIO1);        /* 配置中断引脚 */
  up_mdelay(50);  /* 等待VL53L0X上电稳定 */

  /* ============================================================
   * 第1.5步：LD2410B 毫米波雷达 GPIO 初始化
   * PC0 = 人体存在输入（上拉输入）
   * ============================================================ */

  stm32_configgpio(GPIO_LD2410B_PRESENCE);

  /* ============================================================
   * 第2步：GC9A01显示屏控制引脚初始化
   * CS=PB3, DC=PD7, RST=PB5
   * 初始化前必须先拉低 RST 做硬件复位
   * ============================================================ */

  stm32_configgpio(GPIO_GC9A01_RST);  /* 先拉低复位 */
  up_mdelay(10);                        /* 保持复位 10ms */
  stm32_gpiowrite(GPIO_GC9A01_RST, true);  /* 释放复位 */
  up_mdelay(120);                       /* 等待屏幕初始化完成 */

  stm32_configgpio(GPIO_GC9A01_CS);   /* 片选，默认高 */
  stm32_configgpio(GPIO_GC9A01_DC);   /* 数据/命令，默认低 */

  /* ============================================================
   * 第3步：SPI设备初始化（W25Q Flash + GC9A01）
   * ============================================================ */

#ifdef CONFIG_STM32_SPI1
  stm32_configgpio(GPIO_FLASH_CS);

  if (stm32_spidev_initialize)
    {
      stm32_spidev_initialize();
    }
#endif

  /* ============================================================
   * 第4步：SD卡初始化（1-bit SDIO模式）
   * PC8=D0, PC12=CLK, PD2=CMD
   * ============================================================ */

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_STM32_SDIO)
  ret = stm32_sdio_initialize();
  if (ret < 0)
    {
      serr("ERROR: Failed to initialize SD card: %d\n", ret);
    }
  else
    {
      /* SD卡初始化成功后自动挂载到 /sd */

      ret = nx_mount("/dev/mmcsd0", "/sd", "vfat", 0, NULL);
      if (ret < 0)
        {
          syslog(LOG_WARNING, "WARNING: Failed to mount SD card at /sd: %d\n",
                 ret);
          /* 非致命错误，继续启动 */
        }
    }
#endif

  /* ============================================================
   * 第5步：LED和按键初始化
   * ============================================================ */

#ifdef CONFIG_ARCH_LEDS
  board_autoled_initialize();
#endif

#ifdef CONFIG_INPUT_BUTTONS
  ret = btn_lower_initialize("/dev/buttons");
  if (ret < 0)
    {
      serr("ERROR: Failed to initialize buttons: %d\n", ret);
    }
#endif

  /* ============================================================
   * 第6步：文件系统挂载
   * ============================================================ */

#ifdef CONFIG_FS_PROCFS
  ret = nx_mount(NULL, STM32_PROCFS_MOUNTPOINT, "procfs", 0, NULL);
  if (ret < 0)
    {
      serr("ERROR: Failed to mount procfs at %s: %d\n",
           STM32_PROCFS_MOUNTPOINT, ret);
    }
#endif

#ifdef CONFIG_FS_TMPFS
  ret = nx_mount(NULL, CONFIG_LIBC_TMPDIR, "tmpfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount tmpfs at %s: %d\n",
             CONFIG_LIBC_TMPDIR, ret);
    }
#endif

#ifdef CONFIG_STM32_ROMFS
  ret = stm32_romfs_initialize();
  if (ret < 0)
    {
      serr("ERROR: Failed to mount romfs at %s: %d\n",
           CONFIG_STM32_ROMFS_MOUNTPOINT, ret);
    }
#endif

  /* ============================================================
   * 第7步：USB初始化（如需要）
   * ============================================================ */

#ifdef CONFIG_STM32_OTGFS
  if (stm32_usbinitialize)
    {
      stm32_usbinitialize();
    }
#endif

  return ret;
}
