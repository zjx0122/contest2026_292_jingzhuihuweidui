############################################################################
# desk_health - Makefile
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = desk_health
PRIORITY  = $(CONFIG_EXAMPLES_DESK_HEALTH_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_DESK_HEALTH_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_DESK_HEALTH)

CSRCS = algo/algo_filter.c algo/algo_fusion.c algo/algo_posture.c algo/algo_health.c
CSRCS += tasks/task_radar.c tasks/task_distance.c tasks/task_envirnoment.c
CSRCS += tasks/task_esp32_comm.c tasks/task_display.c
CSRCS += tasks/task_led.c tasks/task_button.c tasks/task_storage.c
CSRCS += protocol/esp32_protocol.c protocol/app_protocol.c
CSRCS += ui/ui_main.c ui/ui_alert.c ui/ui_status.c

MAINSRC = desk_health_main.c

CFLAGS += -I$(APPDIR)/examples/desk_health/include

include $(APPDIR)/Application.mk
