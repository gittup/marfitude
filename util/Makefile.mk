EXTRACFLAGS = `$(SDL_CONFIG) --cflags` $(BDECFLAGS)

TARGET = libmarf

include $(MK)/lang/c.mk

include $(MK)/tgt/staticlib.mk
