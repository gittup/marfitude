PACKAGES = sdl
EXTRACFLAGS += -DMOD_MUSIC
INCDIRS = $(d)/mikmod
LIBS = $(d)/mikmod/libsdl_mikmod.a
include mk/tgt/staticlib.mk

subdir := mikmod
include mk/Recurse.mk
