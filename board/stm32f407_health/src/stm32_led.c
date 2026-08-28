/****************************************************************************
 * boards/arm/stm32/stm32f407_health/src/stm32_led.c
 *
 * 状态指示灯驱动
 * LED_PRESENCE (PD0) — 在位指示
 * LED_ALERT    (PD1) — 告警指示
 * LED_BLE      (PE6) — 蓝牙连接
 ****************************************************************************/

#include <nuttx/config.h>
#include <errno.h>
#include <debug.h>

#include <arch/board/board.h>
#include "stm32.h"
#include "stm32_led.h"

#ifdef CONFIG_STATUS_LED

/* LED 引脚配置表 */

static const uint32_t g_led_pins[] =
{
  GPIO_LED_PRESENCE,  /* LED_PRESENCE = 0 */
  GPIO_LED_ALERT,     /* LED_ALERT    = 1 */
  GPIO_LED_BLE,       /* LED_BLE      = 2 */
};

#define LED_COUNT (sizeof(g_led_pins) / sizeof(g_led_pins[0]))

int bsp_led_init(void)
{
  int i;

  for (i = 0; i < LED_COUNT; i++)
    {
      /* 配置为推挽输出，默认低电平（灭） */

      stm32_configgpio(g_led_pins[i]);
    }

  return OK;
}

void bsp_led_set(bsp_led_id_t led, bool on)
{
  if (led >= 0 && led < LED_COUNT)
    {
      stm32_gpiowrite(g_led_pins[led], on);
    }
}

#endif /* CONFIG_STATUS_LED */