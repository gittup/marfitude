include mk/lib/opengl.mk
include mk/lib/sdl_image.mk
include mk/lib/sdl.mk

INCDIRS = util gmae/sdl_mixer gmae/sdl_mixer/mikmod
EXTRALDFLAGS = -lm
LIBS = util/libmarf.a gmae/sdl_mixer/libsdl_mixer.a
TARGET = marfitude
include mk/tgt/program.mk

subdir := wam
include mk/Recurse.mk
subdir := sdl_mixer
include mk/Recurse.mk
subdir := music
include mk/Recurse.mk
subdir := sounds
include mk/Recurse.mk
subdir := images
include mk/Recurse.mk
subdir := data
include mk/Recurse.mk
subdir := docs
include mk/Recurse.mk

ifeq ($(ARCH),mingw)
subdir := dlls
include mk/Recurse.mk
endif
