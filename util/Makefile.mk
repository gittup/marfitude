include mk/lib/sdl.mk

EXTRACFLAGS = $(BDECFLAGS)
TARGET = marf

include mk/lang/c.mk

include mk/tgt/staticlib.mk
