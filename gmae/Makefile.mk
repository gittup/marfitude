project: $(src_gmae)/marfitude$(BINARYEXT)

$(src_gmae)/marfitude$(BINARYEXT): $(src_gmae)/$(ARCH)/marfitude$(BINARYEXT)
	$(move)

EXTRACFLAGS = `$(SDL_CONFIG) --cflags` $(BDECFLAGS)

ifeq ($(CONFIG_LOG),1)
EXTRACFLAGS += -DLOG
endif
ifeq ($(CONFIG_DEBUG_MEM),1)
EXTRACFLAGS += -DDEBUG_MEM
endif

INCDIRS = $(src_util) $(src_gmae)/$(ARCH) $(src_gmae)/sdl_mixer/mikmod

EXTRALDFLAGS = `$(SDL_CONFIG) --libs` -lm -lSDL_image $(GLLIB) $(GLULIB)

LIBS = $(src_util)/$(ARCH)/libmarf.a $(src_gmae)/sdl_mixer/$(ARCH)/libsdl_mixer.a $(src_gmae)/sdl_mixer/mikmod/$(ARCH)/libsdl_mikmod.a

TARGET = marfitude

include $(MK)/lang/c.mk

AUTODEPS += $(o)/autosounds.d
AUTODEPS += $(o)/autotextures.d

$(o)/autosounds.d: $(d)/gensounds.pl
	$(Q)$(createdir);\
	echo "  AUTODEP $(MKDISPLAY)";\
	(echo "$(dir $@)sounds.h $(dir $@)sndlist.h: $@ $(dir $<)sounds"; echo '	$$(Q)$< $(dir $<) $(ARCH); $$(dotify)') > $@;\
	$< $(dir $<) $(ARCH);\

$(o)/autotextures.d: $(d)/gentextures.pl
	$(Q)$(createdir);\
	echo "  AUTODEP $(MKDISPLAY)";\
	(echo "$(dir $@)textures.h $(dir $@)texlist.h: $< $(dir $<)images"; echo '	$$(Q)$< $(dir $<) $(ARCH); $$(dotify)') > $@;\
	$< $(dir $<) $(ARCH);\

include $(MK)/tgt/program.mk

dirs := wam sdl_mixer
include $(MK)/Recurse.mk
