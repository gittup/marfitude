EXTRACFLAGS = $(SDLCFLAGS) $(BDECFLAGS)

TARGET = libmarf

ifeq ($(CONFIG_DEBUG_MEM),1)
EXTRACFLAGS += -DDEBUG_MEM
endif

include $(MK)/lang/c.mk

include $(MK)/tgt/staticlib.mk
