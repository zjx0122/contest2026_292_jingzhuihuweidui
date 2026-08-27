/****************************************************************************
 * app/smart_home/dev/su03t_drv.c
 * SU-03T 离线语音模块驱动（USART3: PB10=TX, PB11=RX）
 * 波特率：9600bps
 * 通信协议：自定义串口协议
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include "smart_home.h"

static int g_fd = -1;

int su03t_init(void)
{
  struct termios opt;
  g_fd = open(DEV_USART3, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (g_fd < 0) { syslog(LOG_ERR, "SU-03T: open USART3 failed\n"); return -errno; }
  tcgetattr(g_fd, &opt);
  cfsetispeed(&opt, B9600);
  cfsetospeed(&opt, B9600);
  opt.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  opt.c_iflag = 0; opt.c_oflag = 0; opt.c_lflag = 0;
  opt.c_cc[VMIN] = 0; opt.c_cc[VTIME] = 1;
  tcsetattr(g_fd, TCSANOW, &opt);
  syslog(LOG_INFO, "SU-03T init OK on USART3 (PB10=TX, PB11=RX, 9600bps)\n");
  return OK;
}

enum su03t_cmd_e su03t_read_cmd(void)
{
  uint8_t buf[32]; int n;
  if (g_fd < 0) return SU03T_CMD_NONE;
  n = read(g_fd, buf, sizeof(buf));
  if (n <= 0) return SU03T_CMD_NONE;
  /* 根据实际协议解析，这里用简单匹配示例 */
  if (n >= 2)
    {
      switch (buf[0])
        {
          case 0x01: return SU03T_CMD_LIGHT_ON;
          case 0x02: return SU03T_CMD_LIGHT_OFF;
          case 0x03: return SU03T_CMD_FAN_ON;
          case 0x04: return SU03T_CMD_FAN_OFF;
          case 0x05: return SU03T_CMD_READ_TEMP;
          case 0x06: return SU03T_CMD_READ_HUMIDITY;
          case 0x07: return SU03T_CMD_READ_DISTANCE;
          default: break;
        }
    }
  return SU03T_CMD_NONE;
}

int su03t_play_text(const char *text)
{
  if (g_fd < 0 || !text) return -EINVAL;
  return write(g_fd, text, strlen(text));
}
