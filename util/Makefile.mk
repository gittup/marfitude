EXTRACFLAGS = `sdl-config --cflags`

TARGET = libmarf

include $(MK)/lang/c.mk

include $(MK)/tgt/staticlib.mk
