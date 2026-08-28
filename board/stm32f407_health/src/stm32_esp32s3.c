/****************************************************************************
 * boards/arm/stm32/stm32f407_health/src/stm32_esp32s3.c
 *
 * ESP32-S3 串口通信驱动
 * USART3: PB10=TX, PB11=RX, 115200bps
 *
 * ESP32-S3-N16R8:
 *   GPIO18 (U1RXD) → PB10 (USART3_TX)
 *   GPIO17 (U1TXD) → PB11 (USART3_RX)
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <errno.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <arch/board/board.h>
#include "stm32.h"

#ifdef CONFIG_ESP32S3_UART

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ESP32S3_UART_PATH   "/dev/ttyS2"
#define ESP32S3_BAUDRATE    115200
#define ESP32S3_RX_BUF_SIZE 256

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct esp32s3_dev_s
{
  int fd;
  uint8_t rx_buf[ESP32S3_RX_BUF_SIZE];
  bool initialized;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct esp32s3_dev_s g_esp32s3_dev;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int esp32s3_uart_init(struct esp32s3_dev_s *dev)
{
  struct termios opt;
  int ret;

  dev->fd = open(ESP32S3_UART_PATH, O_RDWR | O_NOCTTY);
  if (dev->fd < 0)
    {
      serr("ERROR: 打开 ESP32-S3 串口失败: %d\n", errno);
      return -errno;
    }

  ret = tcgetattr(dev->fd, &opt);
  if (ret < 0)
    {
      serr("ERROR: 获取串口配置失败: %d\n", errno);
      close(dev->fd);
      dev->fd = -1;
      return -errno;
    }

  cfsetispeed(&opt, ESP32S3_BAUDRATE);
  cfsetospeed(&opt, ESP32S3_BAUDRATE);

  opt.c_cflag = ESP32S3_BAUDRATE | CS8 | CLOCAL | CREAD;
  opt.c_iflag = 0;
  opt.c_oflag = 0;
  opt.c_lflag = 0;
  opt.c_cc[VMIN]  = 0;
  opt.c_cc[VTIME] = 10;

  ret = tcsetattr(dev->fd, TCSANOW, &opt);
  if (ret < 0)
    {
      serr("ERROR: 设置串口配置失败: %d\n", errno);
      close(dev->fd);
      dev->fd = -1;
      return -errno;
    }

  tcflush(dev->fd, TCIOFLUSH);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_esp32s3_setup(void)
{
  struct esp32s3_dev_s *dev = &g_esp32s3_dev;
  int ret;

  if (dev->initialized)
    {
      return OK;
    }

  ret = esp32s3_uart_init(dev);
  if (ret < 0)
    {
      serr("ERROR: ESP32-S3 串口初始化失败: %d\n", ret);
      return ret;
    }

  dev->initialized = true;

  sinfo("ESP32-S3 串口初始化成功: %s @ %d bps\n",
        ESP32S3_UART_PATH, ESP32S3_BAUDRATE);
  return OK;
}

int stm32_esp32s3_send(const uint8_t *data, size_t len)
{
  struct esp32s3_dev_s *dev = &g_esp32s3_dev;
  int ret;

  if (!dev->initialized)
    {
      return -ENODEV;
    }

  if (data == NULL || len == 0)
    {
      return -EINVAL;
    }

  ret = write(dev->fd, data, len);
  if (ret < 0)
    {
      serr("ERROR: ESP32-S3 发送数据失败: %d\n", errno);
      return -errno;
    }

  return ret;
}

int stm32_esp32s3_recv(uint8_t *buf, size_t maxlen)
{
  struct esp32s3_dev_s *dev = &g_esp32s3_dev;
  int ret;

  if (!dev->initialized)
    {
      return -ENODEV;
    }

  if (buf == NULL || maxlen == 0)
    {
      return -EINVAL;
    }

  ret = read(dev->fd, buf, maxlen);
  if (ret < 0)
    {
      if (errno == EAGAIN)
        {
          return 0;
        }

      serr("ERROR: ESP32-S3 接收数据失败: %d\n", errno);
      return -errno;
    }

  return ret;
}

int stm32_esp32s3_recv_timeout(uint8_t *buf, size_t maxlen, int timeout_ms)
{
  struct esp32s3_dev_s *dev = &g_esp32s3_dev;
  fd_set readfds;
  struct timeval tv;
  int ret;

  if (!dev->initialized)
    {
      return -ENODEV;
    }

  if (buf == NULL || maxlen == 0)
    {
      return -EINVAL;
    }

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  FD_ZERO(&readfds);
  FD_SET(dev->fd, &readfds);

  ret = select(dev->fd + 1, &readfds, NULL, NULL, &tv);
  if (ret > 0)
    {
      ret = read(dev->fd, buf, maxlen);
      if (ret < 0)
        {
          serr("ERROR: ESP32-S3 接收数据失败: %d\n", errno);
          return -errno;
        }

      return ret;
    }
  else if (ret == 0)
    {
      return -ETIMEDOUT;
    }
  else
    {
      serr("ERROR: ESP32-S3 select 失败: %d\n", errno);
      return -errno;
    }
}

int stm32_esp32s3_flush(void)
{
  struct esp32s3_dev_s *dev = &g_esp32s3_dev;

  if (!dev->initialized)
    {
      return -ENODEV;
    }

  tcflush(dev->fd, TCIFLUSH);
  return OK;
}

bool stm32_esp32s3_is_connected(void)
{
  struct esp32s3_dev_s *dev = &g_esp32s3_dev;
  return dev->initialized && (dev->fd >= 0);
}

#endif /* CONFIG_ESP32S3_UART */
