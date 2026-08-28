/* ui_main.c - 主界面渲染（桌宠+数据条+状态栏） */

#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include "desk_health.h"

#define SCREEN_W 240
#define SCREEN_H 240

/* 桌宠表情绘制（简化版：用文字标识，实际应用替换为位图） */

void ui_init(void)
{
  /* LCD 初始化由 BSP 驱动完成 */
}

void ui_set_face(enum face_type_e face)
{
  /* 切换桌宠表情位图 */
}

void ui_draw_main_screen(struct desk_health_ctx_s *ctx)
{
  /* 实际应用中在此绘制：
   * 1. 顶部状态栏（在位/APP连接图标）
   * 2. 中间桌宠表情区域
   * 3. 底部数据条（温度/湿度/距离/久坐时长）
   */
}
