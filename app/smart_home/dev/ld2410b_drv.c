/****************************************************************************
 * app/smart_home/dev/ld2410b_drv.c
 * LD2410B 毫米波雷达驱动（USART2: PA2=TX, PA3=RX）
 * 协议：帧头FD FC FB FA + 数据长度 + 命令 + 数据 + 帧尾04 03 02 01
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "smart_home.h"

/* LD2410B 帧定义 */

#define LD2410B_FRAME_HEADER    0xfdfcfbfa
#define LD2410B_FRAME_TAIL      0x04030201
#define LD2410B_BUF_SIZE        128

static int g_fd = -1;
static uint8_t g_buf[LD2410B_BUF_SIZE];
static int g_buflen = 0;

int ld2410b_init(void)
{
  struct termios opt;
  g_fd = open(DEV_USART2, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (g_fd < 0) { syslog(LOG_ERR, "LD2410B: open USART2 failed\n"); return -errno; }
  tcgetattr(g_fd, &opt);
  cfsetispeed(&opt, B256000);
  cfsetospeed(&opt, B256000);
  opt.c_cflag = B256000 | CS8 | CLOCAL | CREAD;
  opt.c_iflag = 0; opt.c_oflag = 0; opt.c_lflag = 0;
  opt.c_cc[VMIN] = 0; opt.c_cc[VTIME] = 1;
  tcsetattr(g_fd, TCSANOW, &opt);
  syslog(LOG_INFO, "LD2410B init OK on USART2 (PA2=TX, PA3=RX, 256000bps)\n");
  return OK;
}

int ld2410b_read(struct ld2410b_data_s *data)
{
  int n; uint8_t ch;
  if (!data || g_fd < 0) return -EINVAL;
  while ((n = read(g_fd, &ch, 1)) > 0)
    {
      if (g_buflen < LD2410B_BUF_SIZE) g_buf[g_buflen++] = ch;
      if (g_buflen >= 4 &&
          g_buf[g_buflen-4]==0x04 && g_buf[g_buflen-3]==0x03 &&
          g_buf[g_buflen-2]==0x02 && g_buf[g_buflen-1]==0x01)
        {
          if (g_buflen >= 12 && g_buf[0]==0xfd && g_buf[1]==0xfc &&
              g_buf[2]==0xfb && g_buf[3]==0xfa)
            {
              uint8_t cmd = g_buf[7];
              if (cmd == 0x01)
                {
                  data->target_detected = (g_buf[9] == 0x01);
                  data->distance_cm = g_buf[10] | (g_buf[11] << 8);
                  data->energy = (g_buflen > 12) ? g_buf[12] : 0;
                }
              else
                {
                  data->target_detected = false;
                  data->distance_cm = 0;
                  data->energy = 0;
                }
              g_buflen = 0;
              struct timespec ts;
              clock_gettime(CLOCK_MONOTONIC, &ts);
              data->timestamp = (uint64_t)ts.tv_sec*1000000ULL + ts.tv_nsec/1000ULL;
              return OK;
            }
          g_buflen = 0;
        }
    }
  return -EAGAIN;
}
