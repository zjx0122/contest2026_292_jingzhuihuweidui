/****************************************************************************
 * app/smart_home/service/comm_service.c
 * 通信服务：BLE蓝牙数据收发（UART4: PC10=TX, PC11=RX, AF8）
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <syslog.h>
#include "smart_home.h"

static int g_ble_fd = -1;

int comm_service_init(void)
{
  struct termios opt;
  g_ble_fd = open(DEV_UART4, O_RDWR | O_NOCTTY);
  if (g_ble_fd < 0) { syslog(LOG_ERR, "BLE: open UART4 failed\n"); return -errno; }
  tcgetattr(g_ble_fd, &opt);
  cfsetispeed(&opt, B115200);
  cfsetospeed(&opt, B115200);
  opt.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
  opt.c_iflag = 0; opt.c_oflag = 0; opt.c_lflag = 0;
  tcsetattr(g_ble_fd, TCSANOW, &opt);
  syslog(LOG_INFO, "BLE comm init OK on UART4 (PC10=TX, PC11=RX, AF8, 115200bps)\n");
  return OK;
}

int comm_service_send(const char *data, int len)
{
  if (g_ble_fd < 0 || !data) return -EINVAL;
  return write(g_ble_fd, data, len);
}

int comm_service_recv(char *buf, int maxlen)
{
  if (g_ble_fd < 0 || !buf) return -EINVAL;
  return read(g_ble_fd, buf, maxlen);
}
