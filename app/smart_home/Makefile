############################################################################
# app/smart_home/Makefile
# 智能家居应用构建配置
############################################################################

include $(APPDIR)/Make.defs

# 源文件
CSRCS  = dev/sht30_drv.c
CSRCS += dev/vl53l0x_drv.c
CSRCS += dev/ld2410b_drv.c
CSRCS += dev/su03t_drv.c
CSRCS += dev/gc9a01_drv.c
CSRCS += service/sensor_service.c
CSRCS += service/display_service.c
CSRCS += service/comm_service.c

# 主程序
MAINSRC = app/smart_home_main.c

# 注册为builtin应用
PROGNAME  = smart_home
PRIORITY  = 100
STACKSIZE = 4096
MODULE    = $(CONFIG_PACKAGES_DEMOS_SMART_HOME)

include $(APPDIR)/Application.mk
