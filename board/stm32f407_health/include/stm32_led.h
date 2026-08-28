/****************************************************************************
 * boards/arm/stm32/stm32f407_health/include/stm32_led.h
 *
 * 状态指示灯 BSP API
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32_STM32F407_HEALTH_INCLUDE_STM32_LED_H
#define __BOARDS_ARM_STM32_STM32F407_HEALTH_INCLUDE_STM32_LED_H

#include <nuttx/config.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* LED ID 枚举 */

typedef enum
{
  LED_PRESENCE = 0,  /* PD0 在位指示灯 */
  LED_ALERT    = 1,  /* PD1 告警指示灯 */
  LED_BLE      = 2   /* PE6 蓝牙连接指示灯 */
} bsp_led_id_t;

/****************************************************************************
 * Name: bsp_led_init
 *
 * Description:
 *   初始化所有 LED GPIO 引脚为输出模式，默认灭。
 *
 ****************************************************************************/

int bsp_led_init(void);

/****************************************************************************
 * Name: bsp_led_set
 *
 * Description:
 *   控制对应 LED 亮灭。
 *
 ****************************************************************************/

void bsp_led_set(bsp_led_id_t led, bool on);

#ifdef __cplusplus
}
#endif

#endif /* __BOARDS_ARM_STM32_STM32F407_HEALTH_INCLUDE_STM32_LED_H */