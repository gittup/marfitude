TARGET = libsdl_mikmod

EXTRACFLAGS := $(BDECFLAGS)

INCDIRS := $(d)

include mk/lang/c.mk

include mk/tgt/staticlib.mk
