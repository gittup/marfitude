TARGET = libsdl_mikmod

EXTRACFLAGS := $(BDECFLAGS)

INCDIRS := $(d)

include $(MK)/lang/c.mk

include $(MK)/tgt/staticlib.mk
