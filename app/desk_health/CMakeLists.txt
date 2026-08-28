# desk_health - CMakeLists.txt
if(CONFIG_EXAMPLES_DESK_HEALTH)
  file(GLOB ALGO_SRCS algo/*.c)
  file(GLOB TASK_SRCS tasks/*.c)
  file(GLOB PROTO_SRCS protocol/*.c)
  file(GLOB UI_SRCS ui/*.c)

  nuttx_add_application(
    NAME desk_health
    SRCS desk_health_main.c ${ALGO_SRCS} ${TASK_SRCS} ${PROTO_SRCS} ${UI_SRCS}
    STACKSIZE ${CONFIG_EXAMPLES_DESK_HEALTH_STACKSIZE}
    INCLUDE_DIRECTORIES include)
endif()
