EXTRACFLAGS = $(SDLCFLAGS) -DMOD_MUSIC $(BDECFLAGS)

INCDIRS := $(d)/mikmod

TARGET = libsdl_mixer

include $(MK)/lang/c.mk

include $(MK)/tgt/staticlib.mk

subdir := mikmod
include $(MK)/Recurse.mk
