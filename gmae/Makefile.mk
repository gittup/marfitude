include mk/lib/opengl.mk
include mk/lib/sdl_image.mk
include mk/lib/sdl.mk

EXTRACFLAGS = $(BDECFLAGS)

INCDIRS = cfg util gmae/sdl_mixer gmae/sdl_mixer/mikmod

EXTRALDFLAGS = -lm

LIBS = util/libmarf.a gmae/sdl_mixer/libsdl_mixer.a gmae/sdl_mixer/mikmod/libsdl_mikmod.a

TARGET = marfitude

include mk/lang/c.mk

INSTALL_DIR = $(BINDIR)
INSTALL_LIST = $(o)/marfitude$(BINARYEXT)
include mk/tgt/program.mk

INSTALL_DIR = $(DATADIR)
INSTALL_LIST = Font.png init.cfg
include mk/tgt/data.mk

INSTALL_DIR = $(DATADIR)
INSTALL_LIST = README PROPS UNTESTED TODO README-SDL
include mk/tgt/text.mk

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

ifeq ($(ARCH),mingw)
subdir := dlls
include mk/Recurse.mk
endif
