include mk/lib/sdl.mk

EXTRACFLAGS = $(BDECFLAGS)
TARGET = libmarf

include mk/lang/c.mk

include mk/tgt/staticlib.mk
