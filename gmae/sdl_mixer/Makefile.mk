EXTRACFLAGS = `$(SDL_CONFIG) --cflags` -DMOD_MUSIC

TARGET = libsdl_mixer

include $(MK)/lang/c.mk

include $(MK)/tgt/staticlib.mk

dirs := mikmod
include $(MK)/Recurse.mk
