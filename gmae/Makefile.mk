EXTRACFLAGS = $(SDLCFLAGS) $(BDECFLAGS)

ifeq ($(CONFIG_LOG),1)
EXTRACFLAGS += -DLOG
endif
ifeq ($(CONFIG_DEBUG_MEM),1)
EXTRACFLAGS += -DDEBUG_MEM
endif

INCDIRS = $(src_util) $(src_gmae)/$(ARCH) $(src_gmae)/sdl_mixer $(src_gmae)/sdl_mixer/mikmod

EXTRALDFLAGS = $(SDLLIBS) -lm -lSDL_image $(GLLIBS) $(GLULIBS)

LIBS = $(src_util)/$(ARCH)/libmarf.a $(src_gmae)/sdl_mixer/$(ARCH)/libsdl_mixer.a $(src_gmae)/sdl_mixer/mikmod/$(ARCH)/libsdl_mikmod.a

TARGET = marfitude

include $(MK)/lang/c.mk

INSTALL_DIR = $(BINDIR)
INSTALL_LIST = $(d)/$(ARCH)/marfitude$(BINARYEXT)
include $(MK)/tgt/program.mk

INSTALL_DIR = $(DATADIR)
INSTALL_LIST = Font.png init.cfg
include $(MK)/tgt/data.mk

INSTALL_DIR = $(DATADIR)
INSTALL_LIST = README PROPS UNTESTED TODO README-SDL
include $(MK)/tgt/text.mk

subdir := wam
include $(MK)/Recurse.mk
subdir := sdl_mixer
include $(MK)/Recurse.mk
subdir := music
include $(MK)/Recurse.mk
subdir := sounds
include $(MK)/Recurse.mk
subdir := images
include $(MK)/Recurse.mk

ifeq ($(ARCH),mingw)
subdir := dlls
include $(MK)/Recurse.mk
endif
