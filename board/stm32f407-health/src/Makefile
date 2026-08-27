# Minimal board Makefile

CSRCS  = stm32_boot.c stm32_bringup.c stm32_spi.c

ifeq ($(CONFIG_ARCH_LEDS),y)
CSRCS += stm32_autoleds.c
else
CSRCS += stm32_userleds.c
endif

ifeq ($(CONFIG_BOARDCTL),y)
CSRCS += stm32_appinit.c
endif

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
SRCS  = $(ASRCS) $(CSRCS)
OBJS  = $(AOBJS) $(COBJS)

BIN   = libboard$(LIBEXT)

all: $(BIN)

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
