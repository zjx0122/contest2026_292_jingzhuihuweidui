/****************************************************************************
 * boards/arm/stm32/stm32f407-custom/src/stm32f407-custom.h
 * 适配STM32F407VGT6开发板
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

/* ===== 你的板子只有1个LED：PA1 ===== */

#define GPIO_LED1  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                    GPIO_OUTPUT_CLEAR|GPIO_PORTA|GPIO_PIN1)

/* ===== 用户按键：PA0（外部10K上拉，按下接地）===== */

#define MIN_IRQBUTTON   BUTTON_USER
#define MAX_IRQBUTTON   BUTTON_USER
#define NUM_IRQBUTTONS  1

#define GPIO_BTN_USER   (GPIO_INPUT|GPIO_PULLUP|GPIO_EXTI|GPIO_PORTA|GPIO_PIN0)

/* ===== SPI Flash W25QXX 片选：PA15 ===== */

#define GPIO_FLASH_CS   (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                         GPIO_OUTPUT_SET|GPIO_PORTA|GPIO_PIN15)

/* ===== SD卡检测（如果有卡检测引脚的话）===== */
/* 如果你的SD卡座有卡检测引脚，在这里定义，没有就注释掉 */
/* #define GPIO_SDIO_NCD   (GPIO_INPUT|GPIO_PULLUP|GPIO_EXTI|GPIO_PORTB|GPIO_PIN15) */

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

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_STM32_STM32F407_CUSTOM_SRC_STM32F407_CUSTOM_H */

/* ===== USART1 引脚定义（PA9=TX, PA10=RX）===== */
/* 你的板子只有1个LED(PA1)，其余3个用同一个引脚占位 */
#define GPIO_LED2  GPIO_LED1
#define GPIO_LED3  GPIO_LED1
#define GPIO_LED4  GPIO_LED1
