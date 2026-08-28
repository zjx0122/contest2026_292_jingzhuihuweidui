/* algo_filter.c - 数据滤波 */

#include <nuttx/config.h>
#include <string.h>
#include "desk_health.h"

void filter_init(void)
{
}

float filter_moving_avg(float new_val, float *history, int len)
{
  int i;
  float sum = 0.0f;
  for (i = 0; i < len - 1; i++)
    {
      history[i] = history[i + 1];
      sum += history[i];
    }
  history[len - 1] = new_val;
  sum += new_val;
  return sum / (float)len;
}

uint16_t filter_median(uint16_t *buf, int len)
{
  int i, j;
  uint16_t tmp;
  for (i = 0; i < len - 1; i++)
    for (j = 0; j < len - 1 - i; j++)
      if (buf[j] > buf[j + 1])
        {
          tmp = buf[j]; buf[j] = buf[j+1]; buf[j+1] = tmp;
        }
  return buf[len / 2];
}
