PACKAGES = sdl
EXTRACFLAGS += -DMOD_MUSIC
INCDIRS = $(d)/mikmod
LIBS = $(d)/mikmod/libsdl_mikmod.a
NODOX = 1
EXEC = staticlib
