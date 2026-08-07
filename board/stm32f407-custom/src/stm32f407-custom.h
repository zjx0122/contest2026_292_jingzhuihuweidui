/****************************************************************************
 * boards/arm/stm32/stm32f407-custom/src/stm32f407-custom.h
 * 适配STM32F407VGT6开发板 + 扩展板
 * 硬件：GC9A01屏/SHT30+VL53L0X/LD2410B/SU-03T/BLE/W25Q/SD卡
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32_STM32F407_CUSTOM_SRC_STM32F407_CUSTOM_H
#define __BOARDS_ARM_STM32_STM32F407_CUSTOM_SRC_STM32F407_CUSTOM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <stdint.h>
#include <arch/stm32/chip.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* ===== LED 配置 =====
 * LED1=PD0(人体在位), LED2=PD1(坐姿告警), LED3=PD2(蓝牙状态) - 高电平点亮
 * LED4=PA1 - 低电平点亮
 */

#define GPIO_LED1  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                    GPIO_OUTPUT_CLEAR|GPIO_PORTD|GPIO_PIN0)
#define GPIO_LED2  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                    GPIO_OUTPUT_CLEAR|GPIO_PORTD|GPIO_PIN1)
#define GPIO_LED3  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                    GPIO_OUTPUT_CLEAR|GPIO_PORTD|GPIO_PIN2)
#define GPIO_LED4  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                    GPIO_OUTPUT_SET|GPIO_PORTA|GPIO_PIN1)

/* ===== 用户按键：PA0（外部上拉，按下接地）===== */

#define MIN_IRQBUTTON   BUTTON_USER
#define MAX_IRQBUTTON   BUTTON_USER
#define NUM_IRQBUTTONS  1

#define GPIO_BTN_USER   (GPIO_INPUT|GPIO_PULLUP|GPIO_EXTI|GPIO_PORTA|GPIO_PIN0)

/* ===== GC9A01 LCD 显示屏控制引脚 ===== */

/* LCD_CS=PB3, LCD_DC=PD7, LCD_RST=PB5 */

#define GPIO_GC9A01_CS   (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                          GPIO_OUTPUT_SET|GPIO_PORTB|GPIO_PIN3)
#define GPIO_GC9A01_DC   (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                          GPIO_OUTPUT_CLEAR|GPIO_PORTD|GPIO_PIN7)
#define GPIO_GC9A01_RST  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                          GPIO_OUTPUT_SET|GPIO_PORTB|GPIO_PIN5)

/* ===== W25Q Flash 片选引脚 ===== */

/* W25Q_CS=PA15 */

#define GPIO_FLASH_CS    (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                          GPIO_OUTPUT_SET|GPIO_PORTA|GPIO_PIN15)

/* ===== VL53L0X 传感器引脚 ===== */

/* XSHUT=PC5(输出，拉高使能), GPIO1=PC4(输入，测距完成中断) */

#define GPIO_VL53L0X_XSHUT  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                             GPIO_OUTPUT_CLEAR|GPIO_PORTC|GPIO_PIN5)
#define GPIO_VL53L0X_GPIO1  (GPIO_INPUT|GPIO_PULLUP|GPIO_PORTC|GPIO_PIN4)

/* ===== LD2410B 毫米波雷达 ===== */

/* 人体存在输入: PC0 */

#define GPIO_LD2410B_PRESENCE  (GPIO_INPUT|GPIO_PULLDOWN|GPIO_PORTC|GPIO_PIN0)

/* ===== SDIO SD卡 ===== */

#define HAVE_SDIO  1

#if defined(CONFIG_DISABLE_MOUNTPOINT) || !defined(CONFIG_STM32_SDIO)
#  undef HAVE_SDIO
#endif

#undef  SDIO_MINOR
#define SDIO_MINOR   0
#define SDIO_SLOTNO  0

/* ===== SD卡检测引脚 ===== */
/* 如果你的SD卡座有卡检测引脚，在这里定义 */

/* ===== procfs 挂载点 ===== */

#ifdef CONFIG_FS_PROCFS
#  ifdef CONFIG_NSH_PROC_MOUNTPOINT
#    define STM32_PROCFS_MOUNTPOINT CONFIG_NSH_PROC_MOUNTPOINT
#  else
#    define STM32_PROCFS_MOUNTPOINT "/proc"
#  endif
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

int stm32_bringup(void);

#ifdef CONFIG_STM32_SPI1
void weak_function stm32_spidev_initialize(void);
#endif

#ifdef CONFIG_STM32_OTGFS
void weak_function stm32_usbinitialize(void);
#endif

#if defined(CONFIG_STM32_OTGFS) && defined(CONFIG_USBHOST)
int stm32_usbhost_initialize(void);
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_STM32_SDIO)
int stm32_sdio_initialize(void);
#endif

/* ===== 以太网初始化 ===== */

#ifdef CONFIG_STM32_ETH
int stm32_ethinitialize(void);
#endif

/* ===== CAN 初始化 ===== */

/* stm32_caninitialize is already declared in stm32_can.h */

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_STM32_STM32F407_CUSTOM_SRC_STM32F407_CUSTOM_H */
