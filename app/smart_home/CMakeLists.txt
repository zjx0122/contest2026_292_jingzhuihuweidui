# app/smart_home/CMakeLists.txt
# 智能家居应用 CMake 构建配置

if(CONFIG_PACKAGES_DEMOS_SMART_HOME)
  nuttx_add_application(
    NAME smart_home
    SRCS
      dev/sht30_drv.c
      dev/vl53l0x_drv.c
      dev/ld2410b_drv.c
      dev/su03t_drv.c
      dev/gc9a01_drv.c
      service/sensor_service.c
      service/display_service.c
      service/comm_service.c
      app/smart_home_main.c
    STACKSIZE ${CONFIG_PACKAGES_DEMOS_SMART_HOME_STACKSIZE})
endif()