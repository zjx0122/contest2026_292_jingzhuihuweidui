/****************************************************************************
 * boards/arm/stm32/stm32f407_health/src/stm32_ld2410b.c
 *
 * LD2410B 毫米波雷达传感器驱动
 * UART: USART2 (/dev/ttyS1) PA2=TX, PA3=RX
 * GPIO: PC0 (GPIO_RADAR_PRESENCE) 存在检测
 *
 * 基于 packages/demos/sensor_transfer/sensor.c 移植
 ****************************************************************************/

#include <nuttx/config.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include "stm32.h"
#include "stm32_gpio.h"
#include <arch/board/board.h>

#ifdef CONFIG_SENSORS_LD2410B

#define RADAR_UART_DEV        "/dev/ttyS1"
#define RADAR_RX_CACHE_SIZE   128

#define RADAR_HEADER_0  0xf4
#define RADAR_HEADER_1  0xf3
#define RADAR_HEADER_2  0xf2
#define RADAR_HEADER_3  0xf1
#define RADAR_TAIL_0    0xf8
#define RADAR_TAIL_1    0xf7
#define RADAR_TAIL_2    0xf6
#define RADAR_TAIL_3    0xf5

static int g_radar_fd = -1;
static uint8_t g_radar_rx_cache[RADAR_RX_CACHE_SIZE];
static size_t g_radar_rx_len = 0;
static volatile bool g_radar_presence = false;

static int ld2410b_gpio_isr(int irq, void *context, void *arg)
{
  g_radar_presence = stm32_gpioread(GPIO_RADAR_PRESENCE);
  return OK;
}

static void ld2410b_remove_bytes(size_t count)
{
  if (count >= g_radar_rx_len)
    {
      g_radar_rx_len = 0;
      return;
    }

  memmove(g_radar_rx_cache, &g_radar_rx_cache[count],
          g_radar_rx_len - count);
  g_radar_rx_len -= count;
}

static int ld2410b_read_uart(void)
{
  uint8_t buffer[64];
  ssize_t nread;

  while (1)
    {
      nread = read(g_radar_fd, buffer, sizeof(buffer));
      if (nread > 0)
        {
          if (g_radar_rx_len + nread > RADAR_RX_CACHE_SIZE)
            {
              g_radar_rx_len = 0;
            }

          memcpy(&g_radar_rx_cache[g_radar_rx_len], buffer, nread);
          g_radar_rx_len += nread;
          continue;
        }

      if (nread == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
        {
          break;
        }

      return -EIO;
    }

  return OK;
}

static int ld2410b_parse_frame(const uint8_t *frame,
                               size_t frame_len,
                               bsp_radar_data_t *data)
{
  uint16_t payload_len;
  uint8_t state;

  if (frame == NULL || data == NULL || frame_len < 23)
    return -EINVAL;

  if (frame[0] != RADAR_HEADER_0 || frame[1] != RADAR_HEADER_1 ||
      frame[2] != RADAR_HEADER_2 || frame[3] != RADAR_HEADER_3)
    return -EINVAL;

  payload_len = (uint16_t)frame[4] | ((uint16_t)frame[5] << 8);
  if (frame_len != (size_t)(payload_len + 10))
    return -EINVAL;

  if (frame[frame_len - 4] != RADAR_TAIL_0 ||
      frame[frame_len - 3] != RADAR_TAIL_1 ||
      frame[frame_len - 2] != RADAR_TAIL_2 ||
      frame[frame_len - 1] != RADAR_TAIL_3)
    return -EINVAL;

  if (payload_len < 13)
    return -EINVAL;

  if (frame[6] != 0x01 && frame[6] != 0x02)
    return -EINVAL;

  if (frame[7] != 0xaa)
    return -EINVAL;

  if (frame[6 + payload_len - 2] != 0x55 ||
      frame[6 + payload_len - 1] != 0x00)
    return -EINVAL;

  state = frame[8];
  if (state > 3)
    return -EINVAL;

  data->target_state = state;
  data->has_target = (state != 0);
  data->move_distance_cm = (uint16_t)frame[9] |
                            ((uint16_t)frame[10] << 8);
  data->move_energy = frame[11];
  data->rest_distance_cm = (uint16_t)frame[12] |
                            ((uint16_t)frame[13] << 8);
  data->rest_energy = frame[14];
  return OK;
}

static int ld2410b_parse_frames(bsp_radar_data_t *data)
{
  bsp_radar_data_t latest;
  size_t i;
  size_t frame_len;
  uint16_t payload_len;
  bool found = false;

  while (g_radar_rx_len >= 4)
    {
      for (i = 0; i + 3 < g_radar_rx_len; i++)
        {
          if (g_radar_rx_cache[i] == RADAR_HEADER_0 &&
              g_radar_rx_cache[i + 1] == RADAR_HEADER_1 &&
              g_radar_rx_cache[i + 2] == RADAR_HEADER_2 &&
              g_radar_rx_cache[i + 3] == RADAR_HEADER_3)
            break;
        }

      if (i + 3 >= g_radar_rx_len)
        {
          if (g_radar_rx_len > 3)
            {
              memmove(g_radar_rx_cache,
                      &g_radar_rx_cache[g_radar_rx_len - 3], 3);
              g_radar_rx_len = 3;
            }
          break;
        }

      if (i > 0)
        ld2410b_remove_bytes(i);

      if (g_radar_rx_len < 6)
        break;

      payload_len = (uint16_t)g_radar_rx_cache[4] |
                    ((uint16_t)g_radar_rx_cache[5] << 8);
      if (payload_len < 13 || payload_len > 64)
        {
          ld2410b_remove_bytes(1);
          continue;
        }

      frame_len = payload_len + 10;
      if (g_radar_rx_len < frame_len)
        break;

      if (ld2410b_parse_frame(g_radar_rx_cache, frame_len, &latest) == OK)
        {
          *data = latest;
          found = true;
        }

      ld2410b_remove_bytes(frame_len);
    }

  return found ? OK : -EAGAIN;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_ld2410b_setup(void)
{
  int ret;

  /* 配置 GPIO 存在检测引脚 (PC0) */

  stm32_configgpio(GPIO_RADAR_PRESENCE);
  g_radar_presence = stm32_gpioread(GPIO_RADAR_PRESENCE);

  ret = stm32_gpiosetevent(GPIO_RADAR_PRESENCE, true, true, false,
                            ld2410b_gpio_isr, NULL);
  if (ret < 0)
    {
      serr("ERROR: 配置 LD2410B GPIO 中断失败: %d\n", ret);
      return ret;
    }

  /* 打开 UART 设备 */

  g_radar_fd = open(RADAR_UART_DEV, O_RDWR | O_NONBLOCK);
  if (g_radar_fd < 0)
    {
      serr("ERROR: 打开 %s 失败: %d\n", RADAR_UART_DEV, errno);
      return -errno;
    }

  g_radar_rx_len = 0;

  sinfo("LD2410B 雷达传感器初始化成功 (GPIO=PC0, UART=%s)\n",
        RADAR_UART_DEV);
  return OK;
}

bool stm32_ld2410b_is_present(void)
{
  return g_radar_presence;
}

bool stm32_ld2410b_gpio_read(void)
{
  return stm32_gpioread(GPIO_RADAR_PRESENCE);
}

int stm32_ld2410b_get_data(bsp_radar_data_t *data)
{
  int ret;

  if (data == NULL || g_radar_fd < 0)
    {
      return -EINVAL;
    }

  ret = ld2410b_read_uart();
  if (ret < 0)
    {
      return ret;
    }

  return ld2410b_parse_frames(data);
}

#endif /* CONFIG_SENSORS_LD2410B */