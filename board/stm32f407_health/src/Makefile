include Make.defs

# Add arch-level include paths needed for arm_internal.h etc.
INCLUDES += ${INCDIR_PREFIX}$(TOPDIR)$(DELIM)arch$(DELIM)$(CONFIG_ARCH)$(DELIM)src$(DELIM)chip
INCLUDES += ${INCDIR_PREFIX}$(TOPDIR)$(DELIM)arch$(DELIM)$(CONFIG_ARCH)$(DELIM)src$(DELIM)common
INCLUDES += ${INCDIR_PREFIX}$(TOPDIR)$(DELIM)arch$(DELIM)$(CONFIG_ARCH)$(DELIM)src$(DELIM)arm_m
INCLUDES += ${INCDIR_PREFIX}$(TOPDIR)$(DELIM)arch$(DELIM)$(CONFIG_ARCH)$(DELIM)src$(DELIM)armv7-m
INCLUDES += ${INCDIR_PREFIX}$(TOPDIR)$(DELIM)sched
INCLUDES += ${INCDIR_PREFIX}$(TOPDIR)$(DELIM)arch$(DELIM)$(CONFIG_ARCH)$(DELIM)src$(DELIM)arm
CFLAGS += $(INCLUDES)

CSRCS = stm32_boot.c stm32_bringup.c stm32_spi.c

ifeq ($(CONFIG_ARCH_LEDS),y)
CSRCS += stm32_autoleds.c
else
CSRCS += stm32_userleds.c
endif

ifeq ($(CONFIG_BOARDCTL),y)
CSRCS += stm32_appinit.c
endif

ifeq ($(CONFIG_SENSORS_SHT30),y)
CSRCS += stm32_sht30.c
endif

ifeq ($(CONFIG_SENSORS_VL53L0X),y)
CSRCS += stm32_vl53l0x.c
endif

ifeq ($(CONFIG_LCD_GC9A01),y)
CSRCS += stm32_gc9a01.c
CSRCS += stm32_gc9a01_bsp.c
endif

ifeq ($(CONFIG_STATUS_LED),y)
CSRCS += stm32_led.c
endif

ifeq ($(CONFIG_ESP32S3_UART),y)
CSRCS += stm32_esp32s3.c
endif

CFLAGS += ${INCDIR_PREFIX}$(TOPDIR)$(DELIM)arch$(DELIM)$(CONFIG_ARCH)$(DELIM)src$(DELIM)board$(DELIM)board

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
SRCS  = $(ASRCS) $(CSRCS)
OBJS  = $(AOBJS) $(COBJS)

BIN   = libboard$(LIBEXT)

all: $(BIN)

$(COBJS): %$(OBJEXT) : %.c
	$(call COMPILE, $<, $@)

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(BIN): $(OBJS)
	$(call ARCHIVE, $@, $(OBJS))

context:
	@:

.depend: Makefile $(SRCS)
	$(Q) $(MKDEP) $(DEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) > Make.dep
	$(Q) touch $@

depend: .depend

clean:
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include Make.dep
