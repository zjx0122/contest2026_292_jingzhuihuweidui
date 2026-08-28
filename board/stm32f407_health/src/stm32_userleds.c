/****************************************************************************
 * boards/arm/stm32/stm32f407_health/src/stm32_userleds.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * 用户 LED 接口存根（NuttX userled 框架要求）。
 * 实际 LED 控制通过 stm32_led.c 的 bsp_led_* API 完成。
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>
#include <nuttx/leds/userled.h>

#ifndef CONFIG_ARCH_LEDS

void board_userled_initialize(void)
{
  /* LED 初始化由 bsp_led_init() 完成 */
}

void board_userled(int led, bool ledon)
{
  /* LED 控制由 bsp_led_set() 完成 */
}

void board_userled_all(uint8_t ledset)
{
  /* LED 控制由 bsp_led_set() 完成 */
}

#endif /* !CONFIG_ARCH_LEDS */