EXTRACFLAGS = $(SDLCFLAGS) -DMOD_MUSIC $(BDECFLAGS)

INCDIRS := $(d)/mikmod

TARGET = libsdl_mixer

include mk/lang/c.mk

include mk/tgt/staticlib.mk

subdir := mikmod
include mk/Recurse.mk
