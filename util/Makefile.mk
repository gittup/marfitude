include mk/lib/sdl.mk

INCDIRS = cfg
EXTRACFLAGS = $(BDECFLAGS)
TARGET = libmarf

include mk/lang/c.mk

include mk/tgt/staticlib.mk
