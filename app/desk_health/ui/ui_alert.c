/* ui_alert.c - 预警弹窗 */

#include <nuttx/config.h>
#include <stdio.h>
#include "desk_health.h"

void ui_draw_alert_popup(enum alert_level_e level,
                         enum posture_state_e trigger)
{
  /* 绘制预警弹窗：
   * L1: 桌宠切换提醒表情+提示条
   * L2: 弹出提醒卡片
   * L3: 弹窗+语音图标
   */
}
